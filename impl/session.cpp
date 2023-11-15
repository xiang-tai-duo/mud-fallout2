﻿//
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

#include <unistd.h>

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
            auto _this = reinterpret_cast<class session *>(param);
            auto user = std::string();
            auto password = std::string();
            while (!_this->is_shutdown && (user.empty() || password.empty())) {
                _this->notify(WELCOME_TEXT, STATUS_CODE_UNAUTHORIZED);
                auto json = _this->read();
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
                _this->player = p;
                b = b && _this->notify_stage();
                b = b && _this->notify_options();
                auto command = std::string();
                while (!_this->is_shutdown && b) {
                    if (_this->player->health_point <= _this->player->max_health_point * 0.2) {
                        b = b && _this->notify("警告：当前生命值低于20%");
                    }
                    auto option = utils::json::get_string(_this->read(), JSON_KEY_OPTION);
                    if (option.empty()) {
                        _this->notify("option is empty", STATUS_CODE_NOT_ACCEPT);
                    } else {
                        b = _this->execute_option(option);
                        _this->player->save();
                    }
                }
            } else {
                _this->notify(WRONG_USER_NAME_TEXT);
                pthread_t handle;
                pthread_create(&handle, nullptr, [](void *param) -> void * {
                    close_session(param);
                    return nullptr;
                }, _this->connection_hdl);
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
                json[JSON_KEY_TEXT_TYPE] = TEXT_TYPE_TEXT;
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
    events[JSON_KEY_TEXT].push_back(utils::strings::format("* %s", stage.name.c_str()));
    for (auto &message: stage.messages) {
        events[JSON_KEY_TEXT].push_back(utils::strings::format("  %s", message.c_str()));
    }
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
        auto ordered_json = nlohmann::ordered_json();
        ordered_json[JSON_KEY_OPTIONS] = nlohmann::ordered_json();
        for (auto &o: this->player->options()) {
            ordered_json[JSON_KEY_OPTIONS].push_back(o);
        }
        ordered_json[JSON_KEY_OPTIONS].push_back(translate(MENU_ITEM_NAME_REST, this->language));

        // 地下城小地图绘制
        if (this->player->all_maze_maps.find(this->player->action.name) != this->player->all_maze_maps.end()) {

            // 找出当前舞台使用的地图
            auto maps = nlohmann::ordered_json::array();
            auto current_maze_maps = this->player->all_maze_maps[this->player->action.name];
            for (auto &maze_map: *current_maze_maps) {
                auto map = nlohmann::ordered_json();
                map[JSON_KEY_ENTRANCE] = nlohmann::ordered_json();
                map[JSON_KEY_ENTRANCE][JSON_KEY_X] = maze_map->entrance.first;
                map[JSON_KEY_ENTRANCE][JSON_KEY_Y] = maze_map->entrance.second;
                map[JSON_KEY_EXIT][JSON_KEY_X] = maze_map->exit.first;
                map[JSON_KEY_EXIT][JSON_KEY_Y] = maze_map->exit.second;
                auto coordinates = nlohmann::ordered_json::array();
                for (auto &coordinate: maze_map->grid) {
                    coordinates.push_back(coordinate);
                }
                map[JSON_KEY_MAP] = coordinates;
                maps.push_back(map);
            }

            // 如果用户上一个地下城地图和当前地下城地图名称不同，那么用户切换了地图，需要重置用户的坐标位置
            if (!current_maze_maps->empty() && this->player->action.name != this->player->maze_name) {
                this->player->maze_position_x = (*current_maze_maps)[0]->entrance.first;
                this->player->maze_position_y = (*current_maze_maps)[0]->entrance.second;
                this->player->maze_map_index = 0;
                this->player->maze_name = this->player->action.name;
            }

            // 将用户的坐标信息发送到服务器
            ordered_json[JSON_KEY_MAZE][JSON_KEY_X] = this->player->maze_position_x;
            ordered_json[JSON_KEY_MAZE][JSON_KEY_Y] = this->player->maze_position_y;
            ordered_json[JSON_KEY_MAZE][JSON_KEY_INDEX] = this->player->maze_map_index;
            ordered_json[JSON_KEY_MAZE][JSON_KEY_NAME] = this->player->maze_name;
            ordered_json[JSON_KEY_MAZE][JSON_KEY_MAP] = maps;

            // 从地下城地图中找出当前层的地图
            if (this->player->maze_map_index < current_maze_maps->size()) {

                // 计算当前地图的可视范围，应该从大地图的哪个区域开始进行剪切
                auto current_maze_map = (*current_maze_maps)[this->player->maze_map_index];
                auto crop_left = this->player->maze_position_x - MINI_MAP_RADIUS;
                auto crop_right = this->player->maze_position_x + MINI_MAP_RADIUS;
                auto crop_top = this->player->maze_position_y - MINI_MAP_RADIUS;
                auto crop_bottom = this->player->maze_position_y + MINI_MAP_RADIUS;

                // 左边超出左边界，将左边超出边界的部分添补足右边
                if (crop_left < 0) {
                    crop_right += crop_left * -1;
                    crop_left = 0;
                }

                // 如果右边超出左边界
                if (crop_right < 0) {
                    crop_right = 0;
                }

                // 如果上边超出上边界，那么将上边超出边界的部分添补足下边
                if (crop_top < 0) {
                    crop_bottom += crop_top * -1;
                    crop_top = 0;
                }

                // 如果下边超出下边界
                if (crop_bottom < 0) {
                    crop_bottom = 0;
                }

                // 左边的右边界不能超过宽度
                if (crop_left >= current_maze_map->width()) {
                    crop_left = current_maze_map->width() - 1;
                }

                // 右边的右边界不能超过宽度，将右边超出边界的部分添补足左边
                if (crop_right >= current_maze_map->width()) {
                    crop_left += (current_maze_map->width() - 1) - crop_right;
                    if (crop_left < 0) {
                        crop_left = 0;
                    }
                    crop_right = current_maze_map->width() - 1;
                }

                // 上边的上边界不能超过高度
                if (crop_top >= current_maze_map->height()) {
                    crop_top = current_maze_map->height() - 1;
                }

                // 下边的下边界不能超过高度
                if (crop_bottom >= current_maze_map->height()) {
                    crop_top += (current_maze_map->height() - 1) - crop_bottom;
                    if (crop_top < 0) {
                        crop_top = 0;
                    }
                    crop_bottom = current_maze_map->height() - 1;
                }

                // 生成小地图，默认用墙填充小地图
                auto mini_map = std::vector<std::vector<CELL_TYPE >>(MINI_MAP_RADIUS * 2 + 1, std::vector<CELL_TYPE>(MINI_MAP_RADIUS * 2 + 1, CELL_TYPE::WALL));

                // 从mini_map_left/mini_map_top开始填充到小地图
                auto mini_map_x = 0;
                auto mini_map_y = 0;
                for (auto y = crop_top; y <= crop_bottom; y++, mini_map_y++) {
                    for (auto x = crop_left; x <= crop_right; x++, mini_map_x++) {
                        mini_map[mini_map_y][mini_map_x] = current_maze_map->grid[y][x];

                        // 如果坐标是当前用户的坐标，那么将这个点改为x
                        if (x == this->player->maze_position_x && y == this->player->maze_position_y) {
                            mini_map[mini_map_y][mini_map_x] = CELL_TYPE::POSITION;
                        }
                    }
                    mini_map_x = 0;
                }

                // 将小地图转为json格式
                auto mini_map_json = nlohmann::ordered_json::array();
                for (auto &coordinate: mini_map) {
                    mini_map_json.push_back(coordinate);
                }
                ordered_json[JSON_KEY_MAZE][JSON_KEY_MINI_MAP] = mini_map_json;
            }
        }
        ordered_json[JSON_KEY_TEXT_TYPE] = TEXT_TYPE_OPTIONS;
        b = this->notify(ordered_json);
    }
    return b;
}

bool session::execute_option(const std::string &option) {
    auto b = false;
    if (this->player) {
        auto stage_id = this->player->action.stage_id;
        if (this->player->execute(option) || this->move(option) || this->rest(option)) {
            if (stage_id != this->player->action.stage_id) {
                this->notify_stage();
            }
            if (this->player->is_dead()) {
                auto game_over = std::vector<std::string>();
                game_over.emplace_back("你在一次战斗中不幸遇难，沙尘暴沙漠将你的残骸淹没。");
                game_over.emplace_back("由于缺少水净化芯片来净化水源，阿罗由的人们决定在下一次辐射风暴之前撤离这个地方。");
                this->notify(game_over);
                this->notify(GAME_OVER_TEXT);
                b = false;
            } else {
                b = this->notify_messages();
            }
        } else {
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

bool session::move(const std::string &option) {
    auto b = false;
    if (this->player && !this->player->maze_name.empty() &&
        this->player->all_maze_maps.find(this->player->maze_name) != this->player->all_maze_maps.end()) {
        auto maze_map = this->player->all_maze_maps[this->player->maze_name];
        if (this->player->maze_map_index < maze_map->size()) {
            auto map = (*maze_map)[this->player->maze_map_index];
            if (option == MOVE_LEFT) {
                if (map->is_movable(this->player->maze_position_x - 1, this->player->maze_position_y)) {
                    this->player->maze_position_x--;
                }
                b = true;
            } else if (option == MOVE_RIGHT) {
                if (map->is_movable(this->player->maze_position_x + 1, this->player->maze_position_y)) {
                    this->player->maze_position_x++;
                }
                b = true;
            } else if (option == MOVE_UP) {
                if (map->is_movable(this->player->maze_position_x, this->player->maze_position_y - 1)) {
                    this->player->maze_position_y--;
                }
                b = true;
            } else if (option == MOVE_DOWN) {
                if (map->is_movable(this->player->maze_position_x, this->player->maze_position_y + 1)) {
                    this->player->maze_position_y++;
                }
                b = true;
            }
        }
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

bool session::encounter(int duration) {
    auto b = false;
    if (this->player) {
        b = true;
        auto start_time = std::chrono::high_resolution_clock::now();
        while (b && !this->player->is_dead()) {
            auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - start_time).count();
            auto progress = 100;
            if (duration > 0) {
                progress = milliseconds >= duration ? 100 : (int) (((double) milliseconds / (double) duration) * 100);
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