//
// Created by 黄元镭 on 2023/9/18.
//

#include "session.h"
#include "macros.h"
#include "sha1.h"
#include "userdef.hpp"
#include "websocket.h"
#include <iostream>
#include <vector>

#ifdef MAC
#pragma clang diagnostic push
#pragma ide diagnostic ignored "ConstantConditionsOC"
#endif

#ifdef MAC
#include <unistd.h>
#endif

session::session(websocketpp::server<websocketpp::config::asio> *websocketpp, void *hdl, const std::string &language) {
    this->websocketpp = websocketpp;
    this->connection_hdl = hdl;
    this->language = language;
    this->player = nullptr;
    this->is_shutdown = false;
    this->main_thread = nullptr;
    this->main_loop();
}

session::~session() {
    this->release();
}

void session::main_loop() {
    this->main_thread = new std::thread([](void *param) -> void * {
        if (param) {
            auto _this = reinterpret_cast<class session *>(param);
            auto user = std::string();
            auto password = std::string();
            while (!_this->is_shutdown && (user.empty() || password.empty())) {
                _this->wss(translate(WELCOME_TEXT, _this->language), STATUS_CODE_UNAUTHORIZED);
                auto json = _this->read_client_json(INFINITE);
                user = utils::json::get_string(json, JSON_KEY_USER_NAME);
                password = utils::json::get_string(json, JSON_KEY_PASSWORD);
            }
            if (_this->login(user, password, true)) {
                std::vector<void *> exists_sessions;

                // 检测是否已经登录
                enum_wss([&](std::map<void *, struct WSS *> *exists_wss) -> void {
                    for (auto &wss: *exists_wss) {
                        if (wss.second->session && wss.second->session->player &&
                            wss.second->session->player->name == user &&
                            !(_this->connection_hdl == wss.first && _this == wss.second->session)) {
                            exists_sessions.push_back(wss.first);
                        }
                    }
                });
                if (!exists_sessions.empty()) {
                    for (auto &session: exists_sessions) {
                        std::thread([](void *param) -> void {
                            close_wss_client(param);
                        }, session).detach();
                    }
                }
                auto b = true;
                while (!_this->is_shutdown && b) {
                    if (_this->player->health_point <= _this->player->max_health_point * 0.2) {
                        b = b && _this->wss(translate("警告：当前生命值低于20%", _this->language));
                    }
                    auto option = utils::json::get_string(_this->read_client_json(INFINITE), JSON_KEY_OPTION);
                    if (option.empty()) {
                        _this->wss(translate("option is empty", _this->language), STATUS_CODE_NOT_ACCEPT);
                    } else {
                        b = _this->execute_option(option);
                        _this->player->save();
                    }
                }
            } else {
                _this->wss(translate(WRONG_USER_NAME_TEXT, _this->language));
            }
            std::thread([](void *param) -> void {
                close_wss_client(param);
            }, _this->connection_hdl).detach();
        }
        return nullptr;
    }, this);
}

void session::release() {
    this->release_mutex.lock();
    this->is_shutdown = true;
    if (this->main_thread) {
        this->main_thread->join();
        delete this->main_thread;
        this->main_thread = nullptr;
    }
    this->release_mutex.unlock();
}

nlohmann::ordered_json session::read_client_json(int timeout_milliseconds) {
    auto s = std::string();
    auto start_time = NOW;
    auto is_timeout = false;
    auto ready = false;
    this->reset_messages();
    while (!this->is_shutdown && !ready && !is_timeout) {
        is_timeout = false;
        this->messages_mutex.lock();
        if (!this->messages.empty()) {
            s = this->messages[0];
            this->messages.erase(this->messages.begin());
            ready = true;
        }
        this->messages_mutex.unlock();
        if (ready) {
            break;
        } else if (timeout_milliseconds != INFINITE) {
            is_timeout = utils::datetime::duration(start_time) > timeout_milliseconds;
        }
        if (!is_timeout) {
            utils::threading::sleep(POLLING_MILLISECONDS);
        }
    }
    return s.empty() ? nlohmann::ordered_json() : nlohmann::ordered_json::parse(s);
}

std::string session::read_client_option(int timeout_milliseconds) {
    return utils::json::get_string(this->read_client_json(timeout_milliseconds), JSON_KEY_OPTION);
}

void session::push_message(const std::string &s) {
    this->messages_mutex.lock();
    this->messages.push_back(s);
    this->messages_mutex.unlock();
}

bool session::reset_messages() {
    this->messages_mutex.lock();
    this->messages.clear();
    this->messages_mutex.unlock();
    return true;
}

bool session::login(const std::string &user_name, const std::string &password, bool generate_new) {
    auto password_hash = utils::hash::sha1(password);
    std::ifstream save_file(utils::strings::format("%s/%s.json", SAVE_DIRECTORY_NAME, user_name.c_str()));
    if (save_file.is_open()) {
        std::ostringstream ss;
        ss << save_file.rdbuf();
        save_file.close();
        auto content = ss.str();
        if (content.empty()) {
            generate_new = true;
        } else {
            auto json = nlohmann::ordered_json::parse(ss.str());
            auto character = unit::load(json);
            if (character->password_hash == password_hash) {
                this->player = character;
                this->player->current_stage_event = stage::singleton.find_stage_event(utils::json::get_string(json, PROPERTY_NAME_CURRENT_STAGE_EVENT_NAME));
                if (this->player->current_stage_event.initialized) {

                    // 如果场景是地下城的话，下一次登录应该要在场景的入口
                    if (this->player->current_stage_event.maze.initialized) {
                        this->player->current_stage_event = stage::singleton.find_stage_entrance_event(this->player->current_stage_event.stage_id);
                    }
                } else {
                    this->player->current_stage_event = stage::singleton.entrance();
                }
                this->send_stage_info_to_client();

                // 如果玩家生命值为0，那么把生命值改为1
                this->player->health_point = utils::math::max(this->player->health_point, 1);
                this->execute_option(this->player->current_stage_event.name);
            }
        }
    }
    if (generate_new) {
        this->player = new unit();
        this->player->name = user_name;
        this->player->role = ROLE_PLAYER;
        this->player->password_hash = password_hash;
        this->player->current_stage_event = stage::singleton.entrance();
        this->player->save();
    }
    return this->player != nullptr;
}

bool session::wss(const nlohmann::ordered_json &ordered_json) {
    auto b = false;
    if (this->websocketpp) {
        try {
            auto json = ordered_json;
            if (!json.contains(JSON_KEY_STATUS_CODE)) {
                json[JSON_KEY_STATUS_CODE] = STATUS_CODE_OK;
            }
            if (!json.contains(JSON_KEY_TEXT_TYPE)) {
                json[JSON_KEY_TEXT_TYPE] = UTF8(TEXT_TYPE_TEXT);
            }
            if (this->player) {
                auto status = nlohmann::ordered_json();
                status[JSON_KEY_NAME] = UTF8(this->player->name);
                status[JSON_KEY_LEVEL] = this->player->level;
                status[JSON_KEY_HEALTH_POINT] = this->player->health_point;
                status[JSON_KEY_MAX_HEALTH_POINT] = this->player->max_health_point;
                status[JSON_KEY_POWER] = this->player->power;
                status[JSON_KEY_DEFENSIVE] = this->player->defensive;
                status[JSON_KEY_AGILITY] = this->player->agility;
                status[JSON_KEY_WEAPON] = i18n("空手");
                status[JSON_KEY_ARMOR] = i18n("避难所防护服");
                status[JSON_KEY_EXPERIENCE] = this->player->experience;
                json[JSON_KEY_STATUS] = status;
            }
#if 0
            utils::console::trace(utils::strings::remove_escape_characters(GBK(json.dump(2))));
#endif
            // 每次发送消息之前需要延时，但是如果这行代码放在send_wss_data后面的话
            // 或造成wss阻塞，所以要在send_wss_data之前进行延时操作
            utils::threading::sleep(WEBSOCKET_DELAY_MILLISECONDS);
            send_wss_data(this->connection_hdl, json.dump(2));
            b = true;
        } catch (websocketpp::exception const &e) {
            std::cout << "Echo failed because: "
                      << "(" << e.what() << ")" << std::endl;
            this->release();
        }
    }
    return b;
}

bool session::wss(const std::string &utf8, int status_code) {
    auto ordered_json = nlohmann::ordered_json();
    ordered_json[JSON_KEY_TEXT] = utf8;
    ordered_json[JSON_KEY_STATUS_CODE] = status_code;
    return this->wss(ordered_json);
}

bool session::wss(const std::string &utf8) {
    return this->wss(utf8, STATUS_CODE_OK);
}

bool session::wss(const std::vector<std::string> &utf8) {
    auto ordered_json = nlohmann::ordered_json();
    ordered_json[JSON_KEY_TEXT] = nlohmann::ordered_json::array();
    for (auto &o: utf8) {
        ordered_json[JSON_KEY_TEXT].push_back(o);
    }
    return this->wss(ordered_json);
}

bool session::send_inline_text_to_client(const std::string &utf8) {
    auto ordered_json = nlohmann::ordered_json();
    ordered_json[JSON_KEY_TEXT] = utf8;
    ordered_json[JSON_KEY_TEXT_TYPE] = TEXT_TYPE_INLINE_TEXT;
    return this->wss(ordered_json);
}

bool session::send_stage_info_to_client() {
    auto b = true;
    if (this->player->current_stage_event.initialized) {
        auto stage = stage::singleton.find_stage_entrance_event(this->player->current_stage_event.stage_id);
        auto ordered_json = nlohmann::ordered_json();
        ordered_json[JSON_KEY_TEXT] = nlohmann::ordered_json::array();
        ordered_json[JSON_KEY_TEXT].emplace_back(i18n(utils::strings::format("* %s", stage.name.c_str())));
        for (auto &message: stage.messages) {
            ordered_json[JSON_KEY_TEXT].emplace_back(i18n(utils::strings::format("  %s", message.c_str())));
        }
        b = this->wss(ordered_json);
    }
    return b;
}

bool session::wss() {
    auto b = true;
    if (this->player) {
        auto ordered_json = nlohmann::ordered_json();
        ordered_json[JSON_KEY_TEXT] = nlohmann::ordered_json();
        for (auto &e: this->player->current_stage_event.messages) {
            ordered_json[JSON_KEY_TEXT].push_back(i18n(e));
        }
        b = this->wss(ordered_json);
    }
    return b;
}

bool session::send_options_to_client() {
    auto b = true;
    if (this->player) {
        auto ordered_json = nlohmann::ordered_json();
        ordered_json[JSON_KEY_TEXT] = nlohmann::ordered_json::array();
        for (auto &o: this->player->get_options()) {
            ordered_json[JSON_KEY_TEXT].emplace_back(i18n(o));
        }
        ordered_json[JSON_KEY_TEXT].emplace_back(i18n(MENU_ITEM_NAME_REST));
        ordered_json[JSON_KEY_TEXT_TYPE] = TEXT_TYPE_OPTIONS;
        b = this->wss(ordered_json);
    }
    return b;
}

bool session::execute_option(const std::string &option_name) {
    auto b = false;
    if (this->player) {
        auto stage_id = this->player->current_stage_event.stage_id;
        auto stage_event = stage::singleton.find_stage_event(option_name);
        if (!stage_event.initialized) {
            auto index = utils::strings::atoi(option_name) - 1;
            auto options = this->player->get_options();
            if (index < options.size()) {
                stage_event = stage::singleton.find_stage_event(options[index]);
            }
        }
        if (stage_event.initialized) {
            this->player->current_stage_event = stage_event;
            if (this->player->current_stage_event.maze.initialized && this->player->current_stage_event.maze.floors > 0) {
                this->wss(i18n(utils::strings::format("你已进入%s", this->player->current_stage_event.name.c_str())));
                while (this->player->current_stage_event.maze.current_floor < this->player->current_stage_event.maze.floors) {
                    this->wss(i18n(utils::strings::format("当前位置：%s，%d层", this->player->current_stage_event.name.c_str(), this->player->current_stage_event.maze.current_floor + 1)));
                    if (this->encounter(this->player->current_stage_event.maze.duration * utils::math::random(90, 110) / 100)) {
                        if (this->player->is_dead()) {
                            break;
                        } else {
                            this->player->current_stage_event.maze.current_floor++;
                        }
                    } else {
                        break;
                    }
                }
            }
            for (auto &got: this->player->current_stage_event.got) {
                this->player->add_item(got);
            }
            for (auto &lost: this->player->current_stage_event.lost) {
                this->player->delete_item(lost);
            }
            b = true;
        }
        if (b || this->rest(option_name)) {
            if (stage_id != this->player->current_stage_event.stage_id) {
                this->send_stage_info_to_client();
            }
            if (this->player->is_dead()) {
                auto game_over = std::vector<std::string>();
                game_over.emplace_back(i18n(GAME_OVER_TEXT1));
                game_over.emplace_back(i18n(GAME_OVER_TEXT2));
                this->wss(game_over);
                b = false;
            }
        } else {
            b = this->wss(utils::strings::format(i18n("错误！没有找到该命令：“%s”，可能该命令的执行程序已经在上一次大战中丢失"), option_name.c_str()), STATUS_CODE_NOT_FOUND);
            utils::console::critical("Missing name: '%s', stage: %s(%s)",
                                     option_name.c_str(),
                                     this->player->current_stage_event.stage_id.c_str(),
                                     this->player->current_stage_event.name.c_str());
        }
    }
    if (b) {
        this->send_options_to_client();
    }
    return b;
}

bool session::confirm_message(const std::string &text) {
    auto b = false;
    auto message = text;
    auto option = std::string();
    message += "1. 是";
    message += "2. 否";
    do {
        if (this->wss(i18n(message))) {
            option = this->read_client_option(INFINITE);
            if (option == "1") {
                b = true;
                break;
            } else if (option == "2") {
                b = false;
                break;
            }
        }
    } while (!option.empty());
    return b;
}

bool session::rest(const std::string &option) {
    auto b = false;
    if (utils::strings::starts_with(MENU_ITEM_NAME_REST, option)) {
        b = true;
        while (b && this->player->health_point < this->player->max_health_point) {
            this->player->add_health_point(this->player->health_point_recovery_rate);
            auto native = utils::strings::format("正在休息，输入任意命令结束...　当前生命值：%s/%s",
                                                 utils::strings::itoa(this->player->health_point).c_str(),
                                                 utils::strings::itoa(this->player->max_health_point).c_str());
            b = this->wss(i18n(native));
            if (!this->read_client_json(1000).empty()) {
                break;
            }
        }
    }
    return b;
}

bool session::encounter(int duration) {
    auto b = false;
    if (this->player && duration > 0) {
        b = true;
        auto start_time = NOW;
        auto inline_text_initialized = false;
        while (b && !this->player->is_dead()) {
            if (!inline_text_initialized) {
                b = b && this->wss(i18n(utils::strings::format(MAZE_FLOOR_PROGRESS_TEXT, "0")));
                inline_text_initialized = true;
            }
            auto milliseconds = utils::datetime::duration(start_time);
            auto progress = milliseconds >= duration ? 100 : (int) (((double) milliseconds / (double) duration) * 100);
            auto progress_text = utils::strings::itoa(MIN(progress, 100));
            b = b && this->send_inline_text_to_client(i18n(utils::strings::format(MAZE_FLOOR_PROGRESS_TEXT, progress_text.c_str())));
            if (b) {
                if (progress >= 100) {
                    break;
                } else if (utils::math::random(1, 100) <= ENCOUNTER_ODDS) {
                    auto monsters = this->random_monsters();
                    auto monsters_name = std::string();
                    for (const auto &monster: monsters) {
                        if (!monsters_name.empty()) {
                            monsters_name += ", ";
                        }
                        monsters_name += monster->name;
                    }
                    b = b && this->wss(i18n(utils::strings::format("%s%s", ENCOUNTER_TEXT, monsters_name.c_str())));
                    b = b && this->fight(monsters);
                    if (this->player->is_dead()) {
                        break;
                    }
                    b = b && this->wss(i18n(utils::strings::format(MAZE_FLOOR_PROGRESS_TEXT, progress_text.c_str())));
                    monsters.clear();
                }
                if (!this->read_client_json(ENCOUNTER_POLLING_MILLISECONDS).empty()) {
                    if (this->confirm_message(i18n("你希望立即离开当前地下城吗？"))) {
                        b = false;
                    }
                }
            }
        }
    }
    return b;
}

std::vector<unit *> session::random_monsters() const {
    auto monsters = std::vector<unit *>();
    if (this->player) {
        auto item = stage::singleton.find_stage_entrance_event(this->player->current_stage_event.stage_id);
        if (!item.monsters.empty()) {
            auto monsters_template = std::vector<unit>();
            for (auto &monster: item.monsters) {
                monsters_template.emplace_back(*monster);
            }
            auto monster_count = utils::math::random(1, MAX_ENCOUNTER_MONSTER);
            for (auto i = 0; i < monster_count; i++) {
                auto monster = new unit();
                *monster = monsters_template[utils::math::random(0, (int) monsters_template.size() - 1)];
                auto exists = 0;
                for (auto &e: monsters) {
                    if (utils::strings::starts_with(e->name, monster->name)) {
                        exists++;
                    }
                }
                if (exists > 0) {
                    monster->name += utils::strings::format("%d", exists + 1);
                }
                monsters.emplace_back(monster);
            }
        }
    }
    return monsters;
}

bool session::fight(std::vector<unit *> &unit1, std::vector<unit *> &unit2) {
    auto p = const_cast<session *>(this);
    auto b = true;
    auto get_alive_units = [&](std::vector<unit *> &group) -> std::vector<unit *> {
        std::vector<unit *> living;
        for (auto &e: group) {
            if (!e->is_dead()) {
                living.push_back(e);
            }
        }
        return living;
    };

    auto get_fastest_unit = [&](std::vector<unit *> &unit1, std::vector<unit *> &unit2, bool &is_unit1) -> unit * {
        auto fastest = (class unit *) nullptr;
        auto iterator = std::vector<unit *>::iterator();
        for (auto it = unit1.begin(); it != unit1.end(); it++) {
            if (fastest == nullptr || fastest->agility < (*it)->agility) {
                fastest = *it;
                iterator = it;
                is_unit1 = true;
            }
        }
        for (auto it = unit2.begin(); it != unit2.end(); it++) {
            if (fastest == nullptr || fastest->agility < (*it)->agility) {
                fastest = *it;
                iterator = it;
                is_unit1 = false;
            }
        }
        if (is_unit1) {
            unit1.erase(iterator);
        } else {
            unit2.erase(iterator);
        }
        return fastest;
    };

    auto is_all_units_down = [&](std::vector<unit *> &units) -> bool {
        auto knock_down = 0;
        for (auto &e: units) {
            if (e->is_dead()) {
                knock_down++;
            }
        }
        return knock_down == units.size();
    };

    auto can_fight = [&](std::vector<unit *> &units1, std::vector<unit *> &units2) -> bool {
        return !is_all_units_down(units1) && !is_all_units_down(units2);
    };

    auto fighting = [&](unit *attacker, std::vector<unit *> &defenders) -> bool {
        auto b = true;
        auto _defenders = get_alive_units(defenders);
        if (!_defenders.empty()) {
            auto defender = _defenders.at(0);
            auto damage = (int) ((attacker->power - defender->defensive) * (utils::math::random(90, 110) / 100.0));
            if (damage <= 0) {
                damage = 1;
            }
            auto previous_health_point = defender->health_point;
            defender->add_health_point(damage * -1);

            auto message = std::string(ATTACK_ROUND_TEXT);
            message = utils::strings::replace(message, "{1}", attacker->is_player() ? YOU_TEXT : attacker->name);
            message = utils::strings::replace(message, "{2}", defender->is_player() ? YOU_TEXT : defender->name);
            message = utils::strings::replace(message, "{3}", utils::strings::itoa(damage));
            message = utils::strings::replace(message, "{4}", defender->is_player() ? YOU_TEXT : defender->name);
            message = utils::strings::replace(message, "{5}", utils::strings::itoa(previous_health_point));
            message = utils::strings::replace(message, "{6}", utils::strings::itoa(defender->health_point));
            if (p->wss(i18n(message))) {
                if (defender->is_dead()) {
                    message = KNOCKDOWN_TEXT;
                    message = utils::strings::replace(message, "{1}", defender->is_player() ? YOU_TEXT : defender->name);
                    b = b && p->wss(i18n(message));
                    if (b && attacker->is_player() && defender->experience > 0) {
                        message = GOT_EXPERIENCE_TEXT;
                        message = utils::strings::replace(message, "{1}", utils::strings::itoa(defender->experience));
                        b = b && p->wss(i18n(message));
                        auto previous_level = attacker->level;
                        auto previous_max_health_point = attacker->max_health_point;
                        auto previous_power = attacker->power;
                        auto previous_defensive = attacker->defensive;
                        auto previous_agility = attacker->agility;
                        attacker->add_experience(defender->experience);
                        if (attacker->level != previous_level) {
                            auto send_level_up_message = [&](const std::string &ability_name, int value, int previous_value) -> bool {
                                message = ABILITY_UP_TEXT;
                                message = utils::strings::replace(message, "{1}", ability_name);
                                message = utils::strings::replace(message, "{2}", utils::strings::itoa(value - previous_value));
                                message = utils::strings::replace(message, "{3}", utils::strings::itoa(previous_value));
                                message = utils::strings::replace(message, "{4}", utils::strings::itoa(value));
                                return p->wss(i18n(message));
                            };
                            message = LEVEL_UP_TEXT;
                            b = b && send_level_up_message(ABILITY_NAME_MAX_HEALTH_POINT_TEXT, attacker->max_health_point, previous_max_health_point);
                            b = b && send_level_up_message(ABILITY_NAME_POWER_TEXT, attacker->power, previous_power);
                            b = b && send_level_up_message(ABILITY_NAME_DEFENSIVE_TEXT, attacker->defensive, previous_defensive);
                            b = b && send_level_up_message(ABILITY_NAME_AGILITY_TEXT, attacker->agility, previous_agility);
                        }
                    }
                }
            } else {
                b = false;
            }
        }
        return b;
    };

    // The fastest character is calculated from attack group 1 and attack group 2,
    // Once calculated, the fastest character pointer is removed from the attack group,
    // Next time when the calculation is performing, the character will not be in a repeat battle.
    // It is important to note here that both attack group 1 and attack group 2 create a copy from the
    // original attack group 1 and the original attack group 2 for calculation.
    // If the character belongs to attack group 1, then let him fight against attack group 2,
    // if the character belongs to attack group 2, then let him fight against attack group 1.
    auto units1 = get_alive_units(unit1);
    auto units2 = get_alive_units(unit2);
    auto attackers1 = std::vector<unit *>(units1);
    auto attackers2 = std::vector<unit *>(units2);
    while (b && can_fight(units1, units2)) {
        auto is_units1 = true;
        auto fastest = get_fastest_unit(attackers1, attackers2, is_units1);
        if (fastest) {
            b = fighting(fastest, is_units1 ? units2 : units1);

            // 检查所有单位是否都进攻了
            auto is_attackers_empty = false;
            is_attackers_empty |= attackers1.empty() && attackers2.empty();
            is_attackers_empty |= is_all_units_down(attackers1) && is_all_units_down(attackers2);

            // 所有单位是否都进攻了，检查是否还有存活的单位
            // 如果双方都有存活单位，那么进入下一回合
            if (is_attackers_empty) {
                attackers1 = get_alive_units(units1);
                attackers2 = get_alive_units(units2);
            }
        } else {
            break;
        }
    }
    return b;
}

bool session::fight(std::vector<unit *> &units) {
    std::vector<unit *> group;
    group.emplace_back(this->player);
    return this->fight(group, units);
}

#ifdef MAC
#pragma clang diagnostic pop
#endif