//
// Created by 黄元镭 on 2023/9/18.
//

#include "session.h"
#include "macros.h"
#include "sha1.h"
#include "engine.h"

#ifdef WINDOWS

#include "../../cpp-httplib/httplib.h"

#else

#include <sys/socket.h>
#include <unistd.h>

#define INFINITE (-1)

#endif

#define MAX_RECEIVE_BUFFER_SIZE                 1000
#define POLLING_MILLISECONDS                    10
#define MESSAGE_DELAY_MILLISECONDS              1000
#define FIGHT_LOG_DISPLAY_DELAY_MILLISECONDS    300
#define REST_DELAY_MILLISECONDS                 1000
#define MOVE_DELAY_MILLISECONDS                 200
#define MAX_ENCOUNTER_RAGE                      20
#define MAX_ENCOUNTER_MONSTER                   3
#define MACRO_PLAYER_NAME                       "{PLAYER_NAME}"

string_info::string_info() :
        translate(true), delay(MESSAGE_DELAY_MILLISECONDS) {
}

string_info::string_info(const std::string &format_template) :
        translate(true), delay(MESSAGE_DELAY_MILLISECONDS) {
    this->init(true, MESSAGE_DELAY_MILLISECONDS, format_template);
}

string_info::string_info(const std::vector<std::string> &format_template) :
        translate(true), delay(MESSAGE_DELAY_MILLISECONDS) {
    this->init(true, MESSAGE_DELAY_MILLISECONDS, format_template);
}

string_info::string_info(bool translate_string, const std::string &format_template) :
        translate(true), delay(MESSAGE_DELAY_MILLISECONDS) {
    this->init(translate_string, MESSAGE_DELAY_MILLISECONDS, format_template);
}

string_info::string_info(int message_delay_milliseconds, const std::string &format_template) :
        translate(true), delay(MESSAGE_DELAY_MILLISECONDS) {
    this->init(true, message_delay_milliseconds, format_template);
}

string_info::string_info(bool translate_string, int message_delay_milliseconds, const std::string &format_template) :
        translate(true), delay(MESSAGE_DELAY_MILLISECONDS) {
    this->init(translate_string, message_delay_milliseconds, format_template);
}

string_info::string_info(bool translate_string, int message_delay_milliseconds, const std::vector<std::string> &format_template) :
        translate(true), delay(MESSAGE_DELAY_MILLISECONDS) {
    this->init(translate_string, message_delay_milliseconds, format_template);
}

void string_info::init(bool translate_string, int message_delay_milliseconds, const std::string &format_template) {
    this->translate = translate_string;
    this->delay = message_delay_milliseconds;
    this->format.emplace_back(format_template);
}

void string_info::init(bool translate_string, int message_delay_milliseconds, const std::vector<std::string> &format_template) {
    this->translate = translate_string;
    this->delay = message_delay_milliseconds / utils::math::max((int) format_template.size(), 1);
    this->format = format_template;
}

session::session(int socket) {
    this->socket = socket;
    this->terminal_type = TERMINAL_UNKNOWN;
    this->language = "en";
    this->player = nullptr;
    this->buffer.clear();
    this->socket_mutex = new std::mutex();
    this->init();
}

void session::init() {
    new std::thread([&]() -> void {
        auto b = true;
        while (b) {
            char receive_buffer[MAX_RECEIVE_BUFFER_SIZE * 2] = {0};
            auto bytes_count = recv(this->socket, receive_buffer, ARRAYSIZE(receive_buffer) - 1, 0);
            if (bytes_count == SOCKET_ERROR) {
                this->socket = SOCKET_ERROR;
                b = false;
            } else {
                this->socket_mutex->lock();
                this->buffer.append(receive_buffer, bytes_count);
                while (!this->buffer.empty() && this->buffer.size() > MAX_RECEIVE_BUFFER_SIZE) {
                    this->buffer.erase(this->buffer.begin());
                }
                this->socket_mutex->unlock();
            }
        }
    });
    auto encoding = std::string();
    auto user = std::string();
    auto password = std::string();
#ifdef WINDOWS
    if (this->send_string(0, WELCOME_TEXT) &&
        this->send_string(0, CHOOSE_CONSOLE_ENCODING_TEXT) &&
        this->send_string(0, MENU_ITEM_NAME_GBK) &&
        this->send_string(0, MENU_ITEM_NAME_UTF8) &&
        this->read_line(encoding) &&
        this->send_new_line(session)) {
#else
    if (this->send_string(string_info(0, WELCOME_TEXT))) {
#endif
        if (this->send_string(string_info(0, REQUIRE_USER_NAME_TEXT)) &&
            this->clear_buffer() &&
            this->read_line(user) &&
            this->send_string(string_info(0, REQUIRE_PASSCODE_TEXT)) &&
            this->read_line(password)) {
            if (encoding.starts_with("1")) {
                this->terminal_type = TERMINAL_WINDOWS;
            } else if (encoding.starts_with("2")) {
                this->terminal_type = TERMINAL_MACOS;
            }
            auto p = session::login(user, password);
            if (p) {
                this->player = p;
                this->send_new_line();
                this->send_stage_description();
                if (!player->events.empty()) {
                    this->send_new_line();
                    this->send_events();
                }
                auto b = true;
                auto command = std::string();
                while (b && this->read_line(command)) {
                    b = this->raise_event(command);
                    this->player->save();
                }
            } else {
                send_string(WRONG_USER_NAME_TEXT);
                send_string(GAME_OVER_TEXT);
            }
        }
    }
#ifdef WINDOWS
    closesocket(this->socket);
#else
    close(this->socket);
#endif
}

std::string session::read(int timeout) {
    auto s = std::string();
    auto start_time = std::chrono::high_resolution_clock::now();
    auto is_timeout = false;
    while (!is_timeout) {
        this->socket_mutex->lock();
        if (!this->buffer.empty()) {
            s = this->buffer.at(0);
            this->buffer.erase(this->buffer.begin());
        }
        this->socket_mutex->unlock();
        if (s.empty()) {
            if (timeout != INFINITE) {
                auto elapsed_milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - start_time).count();
                if (elapsed_milliseconds > timeout) {
                    is_timeout = true;
                }
            }
            if (!is_timeout) {
                utils::threading::sleep(POLLING_MILLISECONDS);
            }
        } else {
            break;
        }
    }
    return s;
}

std::string session::read() {
    return this->read(INFINITE);
}

std::string session::read_line(int timeout) {
    auto s = std::string();
    auto start_time = std::chrono::high_resolution_clock::now();
    auto is_timeout = false;
    while (!is_timeout) {
        this->socket_mutex->lock();
        if (!this->buffer.empty()) {
            s += this->buffer.at(0);
            this->buffer.erase(this->buffer.begin());
        }
        this->socket_mutex->unlock();
        if (s.ends_with('\n')) {
            break;
        } else {
            if (timeout != INFINITE) {
                auto elapsed_milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - start_time).count();
                if (elapsed_milliseconds > timeout) {
                    is_timeout = true;
                }
            }
            if (!is_timeout) {
                utils::threading::sleep(POLLING_MILLISECONDS);
            }
        }
    }
    while (!s.empty() && (s.ends_with('\r') || s.ends_with('\n'))) {
        s.erase(s.end() - 1);
    }
    return s;
}

std::string session::read_line() {
    return this->read_line(INFINITE);
}

bool session::read_line(std::string &s) {
    s = this->read_line(INFINITE);
    return !s.empty();
}

bool session::clear_buffer() {
    this->socket_mutex->lock();
    this->buffer.clear();
    this->socket_mutex->unlock();
    return true;
}

character *session::login(const std::string &user_name, const std::string &password) {
    auto p = (character *) nullptr;
    auto password_hash = utils::hash::sha1(password);
    std::ifstream save_file(utils::strings::format("%s/%s.json", SAVE_DIRECTORY_NAME, user_name.c_str()));
    if (save_file.is_open()) {
        std::ostringstream ss;
        ss << save_file.rdbuf();
        save_file.close();
        try {
            auto character = character::load(nlohmann::ordered_json::parse(ss.str()));
            if (character->password_hash == password_hash) {
                p = character;
            }
        } catch (...) {

        }
    } else {
        p = character::create(user_name, password_hash, stages.entrance());
        p->save();
    }
    return p;
}

std::string session::replace_variable(const std::string &s) const {
    auto str = s;
    if (this->player) {
        str = utils::strings::replace(str, MACRO_PLAYER_NAME, utils::strings::format("\x1b[32m%s\x1b[0m", this->player->name.c_str()));
    }
    return str;
}

std::string session::highlight_keywords(const std::string &s) {
    auto str = s;
    for (auto it = mud_fallout2::engine::keywords.begin(); it != mud_fallout2::engine::keywords.end(); it++) {
        str = utils::strings::replace(str, it.key(), utils::strings::format("%s%s\x1b[0m", it.value().get<std::string>().c_str(), it.key().c_str()));
    }
    if (str.starts_with("1.")) { str = "\x1b[32m1.\x1b[0m" + str.substr(2); }
    if (str.starts_with("2.")) { str = "\x1b[32m2.\x1b[0m" + str.substr(2); }
    if (str.starts_with("3.")) { str = "\x1b[32m3.\x1b[0m" + str.substr(2); }
    if (str.starts_with("4.")) { str = "\x1b[32m4.\x1b[0m" + str.substr(2); }
    if (str.starts_with("5.")) { str = "\x1b[32m5.\x1b[0m" + str.substr(2); }
    if (str.starts_with("6.")) { str = "\x1b[32m6.\x1b[0m" + str.substr(2); }
    if (str.starts_with("7.")) { str = "\x1b[32m7.\x1b[0m" + str.substr(2); }
    if (str.starts_with("8.")) { str = "\x1b[32m8.\x1b[0m" + str.substr(2); }
    if (str.starts_with("9.")) { str = "\x1b[32m9.\x1b[0m" + str.substr(2); }
    if (str.starts_with("x.")) { str = "\x1b[32mx.\x1b[0m" + str.substr(2); }
    if (str.starts_with("z.")) { str = "\x1b[32mz.\x1b[0m" + str.substr(2); }
    return str;
}

bool session::send_string(string_info string_info, ...) {

#ifdef WINDOWS
    auto sent_bytes = int(SOCKET_ERROR);
#else
    auto sent_bytes = ssize_t(SOCKET_ERROR);
#endif
    if (this->socket != SOCKET_ERROR) {
        if (string_info.format.empty()) {
            auto message = utils::strings::format("[%s]> \r\n", utils::datetime::now().c_str());
            auto bytes = send(this->socket, message.c_str(), (int) message.size(), 0);
            if (bytes == SOCKET_ERROR) {
                this->socket = SOCKET_ERROR;
            } else {
                sent_bytes += bytes;
            }
        } else {
            for (const auto &format: string_info.format) {
                auto native = std::string();
                if (string_info.translate) {
                    native = translate(format, this->language);
                } else {
                    native = format;
                }
                native = replace_variable(native);
                VA_INIT(string_info, native);
                s = highlight_keywords(s);
#ifdef WINDOWS
                switch (this->terminal_type) {
                        case TERMINAL_MACOS:
                            s = utils::strings::gbk_to_utf8(s);
                            break;
                    }
#endif
                for (auto &line: utils::strings::split(s, '\n')) {
                    auto message = utils::strings::format("[%s]> %s\r\n", utils::datetime::now().c_str(), line.c_str());
                    auto bytes = send(this->socket, message.c_str(), (int) message.size(), 0);
                    if (bytes == SOCKET_ERROR) {
                        this->socket = SOCKET_ERROR;
                        break;
                    } else {
                        sent_bytes += bytes;
                        if (string_info.delay > 0) {
                            utils::threading::sleep(string_info.delay);
                        }
                    }
                }
            }
        }
    }
    return sent_bytes != SOCKET_ERROR;
}

bool session::send_stage_description() {
    return this->send_string(string_info(0, PROPERTY_NAME(this->player->stage))) &&
           this->send_string(string_info(PROPERTY_DESCRIPTION(this->player->stage)));
}

bool session::send_new_line() {
    return this->send_string(string_info());
}

bool session::send_response() {
    auto b = true;
    if (this->player) {
        auto result = PROPERTY_RESPONSE(this->player->events);
        if (!result.empty()) {
            b = this->send_string(result) && this->send_new_line();
        }
    }
    return b;
}

void session::send_events() {
    if (this->player) {
        this->send_string(string_info(0, EVENTS_LIST_TEXT));
        auto events = this->player->get_available_events();
        auto index = 1;
        for (auto it = events.begin(); it != events.end(); it++) {
            this->send_string(string_info(false, 0, "%d. %s"), index++, translate(it.key(), this->language).c_str());
        }
        this->send_string(string_info(0, MENU_ITEM_NAME_STATUS));
        this->send_string(string_info(0, MENU_ITEM_NAME_REST));
    }
}

bool session::send_status(const std::string &command) {
    auto b = false;
    if (MENU_ITEM_NAME_STATUS.starts_with(command)) {
        send_string(string_info("名字：%s"), this->player->name.c_str());
        send_string(string_info("等级：%d"), this->player->level);
        send_string(string_info("生命值：%d"), this->player->health_point);
        send_string(string_info("攻击力：%d"), this->player->power);
        send_string(string_info("防御力：%d"), this->player->defensive);
        send_string(string_info("战斗顺序：%d"), this->player->agility);
        send_string(string_info("武器：长狙击枪"));
        send_string(string_info("防具：防弹背心"));
        send_string(string_info("经验值：%d"), this->player->experience);
        this->send_new_line();
        b = true;
    }
    return b;
}

bool session::raise_event(const std::string &event_index) {
    auto b = true;
    auto event_name = std::string();
    auto events = this->player->get_available_events();
    auto ptr = (char *) nullptr;
    auto i = strtol(event_index.c_str(), &ptr, 10);
    auto index = 1;
    for (auto it = events.begin(); it != events.end(); it++) {
        if (index++ == i) {
            event_name = it.key();
            break;
        }
    }
    auto stage_id = PROPERTY_ID(this->player->stage);
    if (event_name.empty() || this->player->execute(event_name).empty()) {
        if (!this->send_status(event_index) &&
            !this->send_rest(event_index)) {
            this->send_new_line();
            this->send_string(string_info("错误！没有找到该命令：“%s”，可能该命令的执行程序已经在上一次大战中丢失"), event_index.c_str());
            utils::console::critical("Missing name: %s, stage: %s(%s)",
                                     event_index.c_str(),
                                     PROPERTY_ID(this->player->stage).c_str(),
                                     PROPERTY_NAME(this->player->stage).c_str());
        }
    } else {
        if (stage_id != PROPERTY_ID(this->player->stage)) {
            this->send_stage_description();
        }
        if (this->raise_encounter() && this->player->is_dead()) {
            std::vector<std::string> game_over;
            game_over.emplace_back("你在一次战斗中不幸遇难，沙尘暴沙漠将你的残骸淹没。");
            game_over.emplace_back("由于缺少水净化芯片，阿罗由的人们在一次沙尘暴之后，决定撤离这个地方，");
            game_over.emplace_back("人们也渐渐的忘记了你的一切...");
            game_over.emplace_back("游戏结束。");
            send_string(game_over);
            b = false;
        } else {
            this->send_response();
        }
    }
    if (b) {
        this->send_events();
    }
    return b;
}

bool session::send_rest(const std::string &command) {
    auto b = false;
    if (MENU_ITEM_NAME_REST.starts_with(command)) {
        b = true;
        auto rest_count = 0;
        while (b && this->read(REST_DELAY_MILLISECONDS).empty() && this->player->health_point < this->player->max_health_point) {
            this->player->add_health_point(this->player->health_point_recovery_rate);
            auto native = translate("正在休息...　当前生命值：{1}/{2}", this->language);
            native = utils::strings::replace(native, "{1}", utils::strings::itoa(this->player->health_point));
            native = utils::strings::replace(native, "{2}", utils::strings::itoa(this->player->max_health_point));
            b = this->send_string(string_info(false, 0, native));
            rest_count++;
        }
        if (rest_count > 0) {
            this->send_new_line();
        }
        b = true;
    }
    return b;
}

bool session::raise_encounter() {
    auto b = false;
    auto encounter = PROPERTY_ENCOUNTER(this->player->events);
    if (!PROPERTY_MONSTERS(this->player->stage).empty() && !encounter.empty()) {
        auto duration = PROPERTY_DURATION(this->player->events);
        auto start_time = std::chrono::high_resolution_clock::now();
        while (duration > 0 && !this->player->is_dead()) {
            auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - start_time).count();
            auto progress = milliseconds >= duration ? 100 : (int) (((double) milliseconds / (double) duration) * 100);
            this->send_string(string_info(false, MOVE_DELAY_MILLISECONDS, "%s... (%d%%)"), translate(encounter, this->language).c_str(), MIN(progress, 100));
            if (progress >= 100) {
                break;
            } else {
                if (utils::math::random(1, 100) <= MAX_ENCOUNTER_RAGE) {
                    auto monsters = this->generate_random_monsters();
                    auto monsters_name = std::string();
                    for (auto monster: monsters) {
                        if (!monsters_name.empty()) {
                            monsters_name += "，";
                        }
                        monsters_name += monster->name;
                    }
                    this->send_string(string_info(false, MESSAGE_DELAY_MILLISECONDS, "%s%s"), translate("你遇到了：", this->language).c_str(), monsters_name.c_str());
                    this->fight(monsters);
                    for (auto monster: monsters) {
                        delete monster;
                    }
                    monsters.clear();
                }
            }
        }
        this->send_new_line();
        b = true;
    }
    return b;
}

std::vector<character *> session::generate_random_monsters() const {
    auto monsters = std::vector<character *>();
    auto stage_monsters = PROPERTY_MONSTERS(this->player->stage);
    if (!stage_monsters.empty()) {
        auto monsters_template = std::vector<character *>();
        for (auto &stage_monster: stage_monsters) {
            monsters_template.emplace_back(character::create(stage_monster));
        }
        auto monster_count = utils::math::random(1, MAX_ENCOUNTER_MONSTER);
        for (auto i = 0; i < monster_count; i++) {
            auto rnd = utils::math::random(0, (int) monsters_template.size() - 1);
            auto monster = new character(monsters_template[rnd]);
            auto monster_index = 0;
            monster->name = translate(monster->name, this->language);
            for (auto &e: monsters) {
                if (e->name.starts_with(monster->name)) {
                    monster_index++;
                }
            }
            if (monster_index > 0) {
                monster->name += utils::strings::format("%d", monster_index + 1);
            }
            monsters.emplace_back(monster);
        }
        for (auto t: monsters_template) {
            delete t;
        }
        monsters_template.clear();
    }
    return monsters;
}

bool session::fight(std::vector<character *> attackers1, std::vector<character *> attackers2) const {
    auto p = const_cast<session *>(this);
    auto b = true;
    auto find_alive = [&](std::vector<character *> &group) -> std::vector<character *> {
        std::vector<character *> alive;
        for (auto &e: group) {
            if (!e->is_dead()) {
                alive.push_back(e);
            }
        }
        return alive;
    };

    auto find_fastest_character = [&](std::vector<character *> &group1, std::vector<character *> &group2) -> character * {
        auto live1 = find_alive(group1);
        auto live2 = find_alive(group2);
        character *fastest = nullptr;
        for (auto &e: live1) {
            if (fastest == nullptr || fastest->agility < e->agility) {
                fastest = e;
            }
        }
        for (auto &e: live2) {
            if (fastest == nullptr || fastest->agility < e->agility) {
                fastest = e;
            }
        }
        return fastest;
    };

    auto is_all_knock_down = [&](std::vector<character *> &characters) -> bool {
        auto knock_down = 0;
        for (auto &e: characters) {
            if (e->is_dead()) {
                knock_down++;
            }
        }
        return knock_down == characters.size();
    };
    auto is_done = [&](std::vector<character *> &attackers1, std::vector<character *> &attackers2) -> bool {
        return is_all_knock_down(attackers1) || is_all_knock_down(attackers2);
    };

    auto fighting = [&](const character *attacker,
                        std::vector<character *> &attacker_group,
                        std::vector<character *> &defensive_group) -> bool {
        auto b = true;
        auto alive = find_alive(defensive_group);
        if (!alive.empty()) {
            auto damage = (int) ((attacker->power - alive.at(0)->defensive) * (utils::math::random(90, 110) / 100.0));
            if (damage <= 0) {
                damage = 1;
            }
            auto previous_health_point = alive.at(0)->health_point;
            alive.at(0)->add_health_point(damage * -1);

            auto native = translate("{1}发起了攻击，对{2}造成了{3}点伤害，({4}的生命值：{5} -> {6})。", this->language);
            native = utils::strings::replace(native, "{1}", translate(attacker->is_player() ? "你" : attacker->name.c_str(), this->language));
            native = utils::strings::replace(native, "{2}", translate(alive.at(0)->is_player() ? "你" : alive.at(0)->name.c_str(), this->language));
            native = utils::strings::replace(native, "{3}", utils::strings::format("\x1b[33m%d\x1b[0m", damage));
            native = utils::strings::replace(native, "{4}", translate(alive.at(0)->is_player() ? "你" : alive.at(0)->name.c_str(), this->language));
            native = utils::strings::replace(native, "{5}", highlight_health_point(alive.at(0)->max_health_point, previous_health_point));
            native = utils::strings::replace(native, "{6}", highlight_health_point(alive.at(0)));
            if (p->send_string(string_info(false, FIGHT_LOG_DISPLAY_DELAY_MILLISECONDS, native))) {
                if (alive.at(0)->is_dead()) {
                    native = translate("{1}被打倒了。", this->language);
                    native = utils::strings::replace(native, "{1}", translate(alive.at(0)->is_player() ? "你" : alive.at(0)->name.c_str(), this->language));
                    b = p->send_string(string_info(false, FIGHT_LOG_DISPLAY_DELAY_MILLISECONDS, native));
                }
            } else {
                b = false;
            }
        }
        if (b) {
            for (auto it = attacker_group.begin(); it != attacker_group.end(); it++) {
                if (*it == attacker) {
                    attacker_group.erase(it);
                    break;
                }
            }
        }
        return b;
    };

    auto pending_attackers1 = find_alive(attackers1);
    auto pending_attackers2 = find_alive(attackers2);

    while (b && !is_done(attackers1, attackers2)) {
        auto fastest = find_fastest_character(pending_attackers1, pending_attackers2);
        auto is_fastest_belong_group1 = false;
        for (auto &e: attackers1) {
            if (fastest == e) {
                is_fastest_belong_group1 = true;
                break;
            }
        }
        if (is_fastest_belong_group1) {
            b = fighting(fastest, pending_attackers1, attackers2);
        } else {
            b = fighting(fastest, pending_attackers2, attackers1);
        }

        // All pending attackers are executed, or
        // The attacker who is pending for attack may already be knocked down in previous round,
        // so check if the attacker who is knocked down, if the attacker has been knocked out, then find a new character
        if ((pending_attackers1.empty() && pending_attackers2.empty()) ||
            (is_all_knock_down(pending_attackers1) && is_all_knock_down(pending_attackers2))) {
            pending_attackers1 = find_alive(attackers1);
            pending_attackers2 = find_alive(attackers2);
        }
    }
    return b;
}

bool session::fight(const std::vector<character *> &character_group) {
    std::vector<character *> group;
    group.emplace_back(this->player);
    return fight(group, character_group);
}

std::string session::highlight_health_point(int max_health_point, int health_point) {
    auto s = std::string("\x1b[31m0\x1b[0m");
    if (max_health_point > 0) {
        if (100.0 * health_point / max_health_point > 70) {
            s = utils::strings::format("\x1b[32m%d\x1b[0m", health_point);
        } else if (100.0 * health_point / max_health_point > 40) {
            s = utils::strings::format("\x1b[33m%d\x1b[0m", health_point);
        } else {
            s = utils::strings::format("\x1b[31m%d\x1b[0m", health_point);
        }
    }
    return s;
}

std::string session::highlight_health_point(const character *character) {
    return character ? highlight_health_point(character->max_health_point, character->health_point) : highlight_health_point(0, 0);
}


