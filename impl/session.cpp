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
#include <queue>
#include <random>

#pragma clang diagnostic push
#pragma ide diagnostic ignored "ConstantConditionsOC"
#ifdef WINDOWS

#include "../../cpp-httplib/httplib.h"

#else

#include <sys/socket.h>
#include <unistd.h>

#include <utility>

#define INFINITE (-1)

#endif

#define MAX_RECEIVE_BUFFER_SIZE                 1000
#define POLLING_MILLISECONDS                    100
#define ENCOUNTER_MILLISECONDS                  1000
#define FIGHT_LOG_DISPLAY_DELAY_MILLISECONDS    300
#define REST_DELAY_MILLISECONDS                 1000
#define MOVE_DELAY_MILLISECONDS                 200
#define STATUS_DELAY_MILLISECONDS               100
#define MAX_ENCOUNTER_RAGE                      20
#define MAX_ENCOUNTER_MONSTER                   6
#define MACRO_PLAYER_NAME                       "{PLAYER_NAME}"

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
    pthread_create(&this->main_thread, nullptr, [](void *param) -> void * {
        if (param) {
            auto session_ptr = reinterpret_cast<class session *>(param);
            auto user = std::string();
            auto password = std::string();
            while (!session_ptr->is_shutdown && (user.empty() || password.empty())) {
                session_ptr->notify(WELCOME_TEXT, STATUS_CODE_UNAUTHORIZED);
                auto json = session_ptr->read();
                user = utils::json::get_string(json, JSON_KEY_USER_NAME);
                password = utils::json::get_string(json, JSON_KEY_PASSWORD);
            }
            auto p = session::login(user, password, true);
            if (p) {
                std::vector<void *> connected_sessions;
                get_sessions([&](std::map<void *, struct SESSION_INFO *> *sessions) -> void {
                    for (auto &session: *sessions) {
                        if (session.second->engine && session.second->engine->player && session.second->engine->player->name == user) {
                            connected_sessions.push_back(session.first);
                        }
                    }
                });
                if (!connected_sessions.empty()) {
                    for (auto &session: connected_sessions) {
                        pthread_t handle;
                        pthread_create(&handle, nullptr, [](void *param) -> void * {
                            close_session(param);
                            return nullptr;
                        }, session);
                    }
                }
                auto b = true;
                session_ptr->player = p;
                b = b && session_ptr->notify_stage();
                b = b && session_ptr->notify_options();
                auto command = std::string();
                while (!session_ptr->is_shutdown && b) {
                    if (session_ptr->player->health_point <= session_ptr->player->max_health_point * 0.2) {
                        b = b && session_ptr->notify("警告：当前生命值低于20%");
                    }
                    auto option = utils::json::get_string(session_ptr->read(), JSON_KEY_OPTION);
                    if (option.empty()) {
                        session_ptr->notify("option is empty", STATUS_CODE_NOT_ACCEPT);
                    } else {
                        b = session_ptr->execute_option(option);
                        session_ptr->player->save();
                    }
                }
            } else {
                session_ptr->notify(WRONG_USER_NAME_TEXT);
                pthread_t handle;
                pthread_create(&handle, nullptr, [](void *param) -> void * {
                    close_session(param);
                    return nullptr;
                }, session_ptr->connection_hdl);
            }
        }
        return nullptr;
    }, this);
}

void session::release() {
    this->release_mutex.lock();
    this->is_shutdown = true;
    pthread_join(this->main_thread, nullptr);
    this->release_mutex.unlock();
}

nlohmann::ordered_json session::read(int timeout) {
    utils::console::trace(utils::strings::format("nlohmann::ordered_json session::read(%s)", timeout == INFINITE ? "INFINITE" : utils::strings::itoa(timeout).c_str()));
    this->clear();
    auto s = std::string();
    auto start_time = std::chrono::high_resolution_clock::now();
    auto is_timeout = false;
    auto ready = false;
    while (!this->is_shutdown && !ready && !is_timeout) {
        this->messages_mutex.lock();
        if (!this->messages.empty()) {
            s = this->messages[0];
            this->messages.erase(this->messages.begin());
            ready = true;
        }
        this->messages_mutex.unlock();
        if (ready) {
            break;
        } else if (timeout != INFINITE) {
            auto elapsed_milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - start_time).count();
            if (elapsed_milliseconds > timeout) {
                is_timeout = true;
            }
            if (!is_timeout) {
                utils::threading::sleep(POLLING_MILLISECONDS);
            }
        }
    }
    return s.empty() ? nlohmann::ordered_json() : nlohmann::ordered_json::parse(s);
}

nlohmann::ordered_json session::read() {
    return this->read(INFINITE);
}

void session::push_message(const std::string &s) {
    utils::console::trace(utils::strings::format("nlohmann::ordered_json session::push_message(%s)", s.c_str()));
    this->messages_mutex.lock();
    this->messages.push_back(s);
    this->messages_mutex.unlock();
}

bool session::clear() {
    this->messages_mutex.lock();
    this->messages.clear();
    this->messages_mutex.unlock();
    return true;
}

unit *session::login(const std::string &user_name, const std::string &password, bool generate_new) {
    auto p = (unit *)
            nullptr;
    auto password_hash = utils::hash::sha1(password);
    std::ifstream save_file(utils::strings::format("%s/%s.json", SAVE_DIRECTORY_NAME, user_name.c_str()));
    if (save_file.is_open()) {
        std::ostringstream ss;
        ss << save_file.rdbuf();
        save_file.close();
        try {
            auto character = unit::load(nlohmann::ordered_json::parse(ss.str()));
            if (character->password_hash == password_hash) {
                p = character;
            }
        } catch (std::exception &ex) {
            utils::console::critical(ex.what());
        }
    } else if (generate_new) {
        p = new unit();
        p->name = user_name;
        p->password_hash = password_hash;
        p->action = stage::singleton.entrance();
        p->save();
    }
    return p;
}

bool session::notify(const nlohmann::ordered_json &ordered_json) {
    auto b = false;
    if (this->websocketpp) {
        try {
            auto json = ordered_json;
            if (!json.contains(JSON_KEY_STATUS_CODE)) {
                json[JSON_KEY_STATUS_CODE] = STATUS_CODE_OK;
            }
            if (!json.contains(JSON_KEY_TEXT_TYPE)) {
                json[JSON_KEY_TEXT_TYPE] = TEXT_TYPE_NONE;
            }
            if (this->player) {
                auto status = nlohmann::ordered_json();
                status[JSON_KEY_NAME] = this->player->name;
                status[JSON_KEY_LEVEL] = this->player->level;
                status[JSON_KEY_HEALTH_POINT] = this->player->health_point;
                status[JSON_KEY_MAX_HEALTH_POINT] = this->player->max_health_point;
                status[JSON_KEY_POWER] = this->player->power;
                status[JSON_KEY_DEFENSIVE] = this->player->defensive;
                status[JSON_KEY_AGILITY] = this->player->agility;
                status[JSON_KEY_WEAPON] = "空手";
                status[JSON_KEY_ARMOR] = "避难所防护服";
                status[JSON_KEY_EXPERIENCE] = this->player->experience;
                json[JSON_KEY_STATUS] = status;
            }
            send_session_data(this->connection_hdl, json.dump(2));
            b = true;
        } catch (websocketpp::exception const &e) {
            std::cout << "Echo failed because: "
                      << "(" << e.what() << ")" << std::endl;
            this->release();
        }
    }
    return b;
}

bool session::notify(const std::string &text, int status_code) {
    auto ordered_json = nlohmann::ordered_json();
    ordered_json[JSON_KEY_TEXT] = text;
    ordered_json[JSON_KEY_STATUS_CODE] = status_code;
    return this->notify(ordered_json);
}

bool session::notify(const std::string &text) {
    return this->notify(text, STATUS_CODE_OK);
}

bool session::notify(const std::vector<std::string> &text) {
    auto ordered_json = nlohmann::ordered_json();
    ordered_json[JSON_KEY_TEXT] = nlohmann::ordered_json();
    for (auto &e: text) {
        ordered_json[JSON_KEY_TEXT].push_back(e);
    }
    return this->notify(ordered_json);
}

bool session::notify(const char *text) {
    return this->notify(std::string(text));
}

bool session::notify_stage() {
    auto stage = stage::singleton.stage_root(this->player->action.stage_id);
    auto events = nlohmann::ordered_json();
    events[JSON_KEY_TEXT] = nlohmann::ordered_json::array();
    events[JSON_KEY_TEXT].push_back(stage.name);
    events[JSON_KEY_TEXT].push_back(stage.messages);
    return this->notify(events);
}

bool session::notify_messages() {
    auto b = true;
    if (this->player) {
        b = this->notify(this->player->action.messages);
    }
    return b;
}

bool session::notify_options() {
    auto b = true;
    if (this->player) {
        auto options = this->player->options();
        options.push_back(translate(MENU_ITEM_NAME_REST, this->language));

        auto ordered_json = nlohmann::ordered_json();
        ordered_json[JSON_KEY_OPTIONS] = nlohmann::ordered_json();
        for (auto &o: options) {
            ordered_json[JSON_KEY_OPTIONS].push_back(o);
        }
        b = this->notify(ordered_json);
    }
    return b;
}

bool session::execute_option(const std::string &option) {
    auto b = false;
    if (this->player) {
        auto stage_id = this->player->action.stage_id;
        if (this->player->execute(option)) {
            if (stage_id != this->player->action.stage_id) {
                this->notify_stage();
            }
            if (this->encounter() && this->player->is_dead()) {
                auto game_over = std::vector<std::string>();
                game_over.emplace_back("你在一次战斗中不幸遇难，沙尘暴沙漠将你的残骸淹没。");
                game_over.emplace_back("由于缺少水净化芯片来净化水源，阿罗由的人们决定在下一次辐射风暴之前撤离这个地方。");
                this->notify(game_over);
                this->notify(GAME_OVER_TEXT);
                b = false;
            } else {
                b = this->notify_messages();
            }
        } else if (!this->rest(option)) {
            b = this->notify(utils::strings::format("错误！没有找到该命令：“%s”，可能该命令的执行程序已经在上一次大战中丢失", option.c_str()), STATUS_CODE_NOT_FOUND);
            utils::console::critical("Missing name: %s, stage: %s(%s)",
                                     option.c_str(),
                                     this->player->action.stage_id.c_str(),
                                     this->player->action.name.c_str());
        }
    }
    if (b) {
        this->notify_options();
    }
    return b;
}

bool session::rest(const std::string &option) {
    auto b = false;
    if (utils::strings::starts_with(MENU_ITEM_NAME_REST, option)) {
        b = true;
        while (b && this->player->health_point < this->player->max_health_point) {
            this->player->add_health_point(this->player->health_point_recovery_rate);
            auto native = translate("正在休息，输入任意命令结束...　当前生命值：{1}/{2}", this->language);
            native = utils::strings::replace(native, "{1}", utils::strings::itoa(this->player->health_point));
            native = utils::strings::replace(native, "{2}", utils::strings::itoa(this->player->max_health_point));
            b = this->notify(native);
            if (!this->read(1000).empty()) {
                break;
            }
        }
    }
    return b;
}

bool session::encounter() {
    auto b = false;
    if (this->player && this->player->action.encounter) {
        b = true;
        auto start_time = std::chrono::high_resolution_clock::now();
        while (b && !this->player->is_dead()) {
            auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - start_time).count();
            auto progress = 100;
            if (this->player->action.duration > 0) {
                progress = milliseconds >= this->player->action.duration ? 100 : (int) (((double) milliseconds / (double) this->player->action.duration) * 100);
            }
            b = b && this->notify(utils::strings::format(
                    "%s... (%s%%)",
                    translate(ENCOUNTER_TEXT, this->language).c_str(),
                    utils::console::yellow(utils::strings::itoa(MIN(progress, 100))).c_str()));
            if (progress >= 100) {
                break;
            } else if (utils::math::random(1, 100) <= MAX_ENCOUNTER_RAGE) {
                auto monsters = this->random_monsters();
                auto monsters_name = std::string();
                for (const auto &monster: monsters) {
                    if (!monsters_name.empty()) {
                        monsters_name += "，";
                    }
                    monsters_name += utils::console::yellow(monster->name);
                }
                b = b && this->notify(utils::strings::format("%s%s", translate(ENCOUNTER_TEXT, this->language).c_str(), monsters_name.c_str()));
                b = b && this->fight(monsters);
                monsters.clear();
            }
            utils::threading::sleep(ENCOUNTER_MILLISECONDS);
        }
    }
    return b;
}

std::vector<unit *> session::random_monsters() const {
    auto monsters = std::vector<unit *>();
    if (this->player) {
        auto stage_root = stage::singleton.stage_root(this->player->action.stage_id);
        if (!stage_root.monsters.empty()) {
            auto monsters_template = std::vector<unit>();
            for (auto &monster: stage_root.monsters) {
                monsters_template.emplace_back(*monster);
            }
            auto monster_count = utils::math::random(1, MAX_ENCOUNTER_MONSTER);
            for (auto i = 0; i < monster_count; i++) {
                auto monster = new unit();
                *monster = monsters_template[utils::math::random(0, (int) monsters_template.size() - 1)];
                monster->name = translate(monster->name, this->language);
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
    auto extract_living = [&](std::vector<unit *> &group) -> std::vector<unit *> {
        std::vector<unit *> living;
        for (auto &e: group) {
            if (!e->is_dead()) {
                living.push_back(e);
            }
        }
        return living;
    };

    auto extract_fastest = [&](std::vector<unit *> &unit1, std::vector<unit *> &unit2, bool &is_unit1) -> unit * {
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

    auto is_all_unit_down = [&](std::vector<unit *> &units) -> bool {
        auto knock_down = 0;
        for (auto &e: units) {
            if (e->is_dead()) {
                knock_down++;
            }
        }
        return knock_down == units.size();
    };
    auto is_fight_end = [&](std::vector<unit *> &units1, std::vector<unit *> &units2) -> bool {
        return is_all_unit_down(units1) || is_all_unit_down(units2);
    };

    auto fighting = [&](unit *attacker, std::vector<unit *> &defenders) -> bool {
        auto b = true;
        auto _defenders = extract_living(defenders);
        if (!_defenders.empty()) {
            auto defender = _defenders.at(0);
            auto damage = (int) ((attacker->power - defender->defensive) * (utils::math::random(90, 110) / 100.0));
            if (damage <= 0) {
                damage = 1;
            }
            auto previous_health_point = defender->health_point;
            defender->add_health_point(damage * -1);

            auto native = translate(ATTACK_ROUND_TEXT, this->language);
            native = utils::strings::replace(native, "{1}", utils::console::yellow(translate(attacker->is_player() ? YOU_TEXT : attacker->name, this->language)));
            native = utils::strings::replace(native, "{2}", utils::console::yellow(translate(defender->is_player() ? YOU_TEXT : defender->name, this->language)));
            native = utils::strings::replace(native, "{3}", utils::console::yellow(utils::strings::itoa(damage)));
            native = utils::strings::replace(native, "{4}", translate(defender->is_player() ? YOU_TEXT : defender->name, this->language));
            native = utils::strings::replace(native, "{5}", utils::strings::itoa(previous_health_point));
            native = utils::strings::replace(native, "{6}", utils::strings::itoa(defender->health_point));
            if (p->notify(native)) {
                if (defender->is_dead()) {
                    native = translate(KNOCKDOWN_TEXT, this->language);
                    native = utils::strings::replace(native, "{1}", translate(defender->is_player() ? YOU_TEXT : defender->name, this->language));
                    b = b && p->notify(native);
                    if (b && attacker->is_player() && defender->experience > 0) {
                        native = utils::console::green(translate(GOT_EXPERIENCE_TEXT, this->language));
                        native = utils::strings::replace(native, "{1}", utils::console::yellow(utils::strings::itoa(defender->experience)) + "\x1b[32m");
                        b = b && p->notify(native);
                        auto previous_level = attacker->level;
                        auto previous_max_health_point = attacker->max_health_point;
                        auto previous_power = attacker->power;
                        auto previous_defensive = attacker->defensive;
                        auto previous_agility = attacker->agility;
                        attacker->add_experience(defender->experience);
                        if (attacker->level != previous_level) {
                            auto report_level_detail = [&](const std::string &ability_name, int value, int previous_value) -> bool {
                                native = translate(ABILITY_UP_TEXT, this->language);
                                native = utils::strings::replace(native, "{1}", utils::console::yellow(translate(ability_name, this->language)) + "\x1b[32m");
                                native = utils::strings::replace(native, "{2}", utils::console::yellow(utils::strings::itoa(value - previous_value)) + "\x1b[32m");
                                native = utils::strings::replace(native, "{3}", utils::console::yellow(utils::strings::itoa(previous_value)) + "\x1b[32m");
                                native = utils::strings::replace(native, "{4}", utils::console::yellow(utils::strings::itoa(value)) + "\x1b[32m");
                                return p->notify(native);
                            };
                            native = utils::console::green(translate(LEVEL_UP_TEXT, this->language));
                            b = b && report_level_detail(ABILITY_NAME_MAX_HEALTH_POINT_TEXT, attacker->max_health_point, previous_max_health_point);
                            b = b && report_level_detail(ABILITY_NAME_POWER_TEXT, attacker->power, previous_power);
                            b = b && report_level_detail(ABILITY_NAME_DEFENSIVE_TEXT, attacker->defensive, previous_defensive);
                            b = b && report_level_detail(ABILITY_NAME_AGILITY_TEXT, attacker->agility, previous_agility);
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
    auto units1 = extract_living(unit1);
    auto units2 = extract_living(unit2);
    auto attackers1 = std::vector<unit *>(units1);
    auto attackers2 = std::vector<unit *>(units2);
    while (b && !is_fight_end(units1, units2)) {
        auto is_unit1 = true;
        auto fastest = extract_fastest(attackers1, attackers2, is_unit1);
        if (fastest) {
            b = fighting(fastest, is_unit1 ? units2 : units1);

            // All pending attackers are executed or the attacker who is pending for attack may already be knocked down in previous round,
            // so check if the attacker who is knocked down, if the attacker has been knocked out, then find a new character
            if ((attackers1.empty() && attackers2.empty()) || (is_all_unit_down(attackers1) && is_all_unit_down(attackers2))) {
                attackers1 = extract_living(units1);
                attackers2 = extract_living(units2);
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

#pragma clang diagnostic pop