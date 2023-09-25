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

#include <utility>

#define INFINITE (-1)

#endif

#define MAX_RECEIVE_BUFFER_SIZE                 1000
#define POLLING_MILLISECONDS                    10
#define MESSAGE_DELAY_MILLISECONDS              1000
#define FIGHT_LOG_DISPLAY_DELAY_MILLISECONDS    300
#define REST_DELAY_MILLISECONDS                 1000
#define MOVE_DELAY_MILLISECONDS                 200
#define STATUS_DELAY_MILLISECONDS               100
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
    this->language = "zh";
    this->player = nullptr;
    this->socket_buffer.clear();
    this->socket_mutex = new std::mutex();
#ifdef MAC
    // https://blog.csdn.net/qq_52572621/article/details/128720889
    if (this->socket != SOCKET_ERROR) {
        this->send_string(string_info(0, "\x1b[2J\x1b[1;1H"));
        this->send_string(string_info(0, "\x1b[33m\x1b[5mCONTROL ECHO TEST\x1b[0m"));
        this->send_string(string_info(0, utils::console::generate_colors_string()));
    }
#endif
    this->main_loop();
}

void session::main_loop() {
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
                this->socket_buffer.append(receive_buffer, bytes_count);
                while (!this->socket_buffer.empty() && this->socket_buffer.size() > MAX_RECEIVE_BUFFER_SIZE) {
                    this->socket_buffer.erase(this->socket_buffer.begin());
                }
                this->socket_mutex->unlock();
            }
        }
    });
    auto encoding = std::string();
    auto user = std::string();
    auto password = std::string();
    while (this->send_string(string_info(0, CHOOSE_CONSOLE_ENCODING_TEXT)) &&
           this->send_string(string_info(0, MENU_ITEM_NAME_GBK)) &&
           this->send_string(string_info(0, MENU_ITEM_NAME_UTF8)) &&
           this->send_string(string_info(0, MENU_ITEM_NAME_WEB)) &&
           this->read_line(encoding)) {
        if (encoding.starts_with("1")) {
            this->terminal_type = TERMINAL_WINDOWS;
        } else if (encoding.starts_with("2")) {
            this->terminal_type = TERMINAL_MACOS;
        } else if (encoding.starts_with("3")) {
            this->terminal_type = TERMINAL_WEB;
        } else {
            this->send_string(string_info(0, "\x1b[5m"));
        }
        if (this->terminal_type != TERMINAL_UNKNOWN) {
            break;
        }
    }
    if (this->terminal_type != TERMINAL_UNKNOWN &&
        this->send_string(string_info(0, WELCOME_TEXT)) &&
        this->send_string(string_info(0, REQUIRE_USER_NAME_TEXT)) &&
        this->read_line(user) &&
        this->send_string(string_info(0, REQUIRE_PASSCODE_TEXT)) &&
        this->read_line(password)) {
        auto p = session::login(user, password);
        if (p) {
            auto b = true;
            this->player = p;
            this->send_new_line();
            this->send_stage_description();
            if (!player->events.empty()) {
                b = b && this->send_new_line();
                b = b && this->send_events();
            }
            if (player->health_point <= player->max_health_point * 0.2) {
                b = b && this->send_string(utils::console::red("警告：当前生命值低于20%%"));
            }
            auto command = std::string();
            while (b && this->read_line(command)) {
                b = this->raise_event(command);
                this->player->save();
                if (player->health_point <= player->max_health_point * 0.2) {
                    b = b && this->send_string(utils::console::red("警告：当前生命值低于20%%"));
                }
            }
        } else {
            send_string(WRONG_USER_NAME_TEXT);
            send_string(GAME_OVER_TEXT);
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
    this->clear_socket_buffer();
    while (!is_timeout) {
        this->socket_mutex->lock();
        if (!this->socket_buffer.empty()) {
            s = this->socket_buffer.at(0);
            this->socket_buffer.erase(this->socket_buffer.begin());
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
    this->clear_socket_buffer();
    while (!is_timeout) {
        this->socket_mutex->lock();
        if (!this->socket_buffer.empty()) {
            s += this->socket_buffer.at(0);
            this->socket_buffer.erase(this->socket_buffer.begin());
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

bool session::clear_socket_buffer() {
    this->socket_mutex->lock();
    this->socket_buffer.clear();
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
                for (auto &line: utils::strings::split(s, '\n')) {
                    auto message = utils::strings::format("[%s]> %s\r\n", utils::datetime::now().c_str(), line.c_str());
                    if (this->terminal_type == TERMINAL_WEB) {
                        message = utils::console::clear_terminal_control_symbol(message);
                    }
#ifdef WINDOWS
                    switch (this->terminal_type) {
                        case TERMINAL_WINDOWS:
                            message = utils::encoding::utf8_to_gbk(message);
                            break;
                    }
#endif
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

bool session::send_events() {
    auto b = true;
    if (this->player) {
        this->send_string(string_info(0, EVENTS_LIST_TEXT));
        auto events = this->player->get_available_events();
        auto index = 1;
        for (auto it = events.begin(); it != events.end(); it++) {
            b = b && this->send_string(string_info(false, 0, "%s. %s"),
                                       utils::console::yellow(utils::strings::itoa(index++)).c_str(),
                                       translate(it.key(), this->language).c_str());
        }
        b = b && this->send_string(string_info(0, utils::strings::replace(MENU_ITEM_NAME_STATUS, "x.", "\x1b[33mx\x1b[0m.")));
        b = b && this->send_string(string_info(0, utils::strings::replace(MENU_ITEM_NAME_REST, "z.", "\x1b[33mz\x1b[0m.")));
    }
    return b;
}

bool session::send_status(const std::string &command) {
    auto b = false;
    if (MENU_ITEM_NAME_STATUS.starts_with(command)) {
        this->send_string(string_info(STATUS_DELAY_MILLISECONDS, "名字：%s"), utils::console::yellow(this->player->name).c_str());
        this->send_string(string_info(STATUS_DELAY_MILLISECONDS, "等级：%s"), utils::console::yellow(utils::strings::itoa(this->player->level)).c_str());
        this->send_string(string_info(STATUS_DELAY_MILLISECONDS, "生命值：%s"), utils::console::yellow(utils::strings::itoa(this->player->health_point)).c_str());
        this->send_string(string_info(STATUS_DELAY_MILLISECONDS, "最大生命值：%s"), utils::console::yellow(utils::strings::itoa(this->player->max_health_point)).c_str());
        this->send_string(string_info(STATUS_DELAY_MILLISECONDS, "攻击力：%s"), utils::console::yellow(utils::strings::itoa(this->player->power)).c_str());
        this->send_string(string_info(STATUS_DELAY_MILLISECONDS, "防御力：%s"), utils::console::yellow(utils::strings::itoa(this->player->defensive)).c_str());
        this->send_string(string_info(STATUS_DELAY_MILLISECONDS, "战斗顺序：%s"), utils::console::yellow(utils::strings::itoa(this->player->agility)).c_str());
        this->send_string(string_info(STATUS_DELAY_MILLISECONDS, "武器：\x1b[33m空手\x1b[0m"));
        this->send_string(string_info(STATUS_DELAY_MILLISECONDS, "防具：\x1b[33m避难所防护服\x1b[0m"));
        this->send_string(string_info(STATUS_DELAY_MILLISECONDS, "经验值：%s"), utils::console::yellow(utils::strings::itoa(this->player->experience)).c_str());
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
            auto game_over = std::vector<std::string>();
            game_over.emplace_back("你在一次战斗中不幸遇难，沙尘暴沙漠将你的残骸淹没。");
            game_over.emplace_back("由于缺少水净化芯片来净化水源，阿罗由的人们决定在下一次辐射风暴之前撤离这个地方，");
            game_over.emplace_back("同时人们也渐渐的忘记了你的一切...");
            this->send_string(game_over);
            this->send_string(utils::console::red(GAME_OVER_TEXT));
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
        while (b && this->read(REST_DELAY_MILLISECONDS).empty()) {
            this->player->add_health_point(this->player->health_point_recovery_rate);
            auto native = translate("生命值已满，输入任意命令结束...", this->language);
            if (this->player->health_point < this->player->max_health_point) {
                native = translate("正在休息，输入任意命令结束...　当前生命值：{1}/{2}", this->language);
            }
            native = utils::strings::replace(native, "{1}", highlight_health_point(this->player->max_health_point, this->player->health_point));
            native = utils::strings::replace(native, "{2}", utils::console::yellow(utils::strings::itoa(this->player->max_health_point)));
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
            this->send_string(string_info(false, MOVE_DELAY_MILLISECONDS, "%s... (%s%%)"),
                              translate(encounter, this->language).c_str(),
                              utils::console::yellow(utils::strings::itoa(MIN(progress, 100))).c_str());
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
                        monsters_name += utils::console::yellow(monster->name);
                    }
                    this->send_string(string_info(false, MESSAGE_DELAY_MILLISECONDS, "%s%s"), translate(ENCOUNTER_TEXT, this->language).c_str(), monsters_name.c_str());
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

bool session::fight(std::vector<character *> characters1, std::vector<character *> characters2) const {
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

    auto pickup_fastest_character = [&](std::vector<character *> &characters1, std::vector<character *> &characters2, bool &is_characters1) -> character * {
        auto fastest = (character *) nullptr;
        auto iterator = std::vector<character *>::iterator();
        for (auto it = characters1.begin(); it != characters1.end(); it++) {
            if (fastest == nullptr || fastest->agility < (*it)->agility) {
                fastest = *it;
                iterator = it;
                is_characters1 = true;
            }
        }
        for (auto it = characters2.begin(); it != characters2.end(); it++) {
            if (fastest == nullptr || fastest->agility < (*it)->agility) {
                fastest = *it;
                iterator = it;
                is_characters1 = false;
            }
        }
        if (is_characters1) {
            characters1.erase(iterator);
        } else {
            characters2.erase(iterator);
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
    auto is_fight_done = [&](std::vector<character *> &attackers1, std::vector<character *> &attackers2) -> bool {
        return is_all_knock_down(attackers1) || is_all_knock_down(attackers2);
    };

    auto fighting = [&](character *attacker, std::vector<class character *> &defenders) -> bool {
        auto b = true;
        auto _defenders = find_alive(defenders);
        if (!_defenders.empty()) {
            auto defender = _defenders.at(0);
            auto damage = (int) ((attacker->power - defender->defensive) * (utils::math::random(90, 110) / 100.0));
            if (damage <= 0) {
                damage = 1;
            }
            auto previous_health_point = defender->health_point;
            defender->add_health_point(damage * -1);

            auto native = translate(ATTACK_ROUND_TEXT, this->language);
            native = utils::strings::replace(native, "{1}", utils::console::yellow(translate(attacker->is_player() ? YOU_TEXT : attacker->name.c_str(), this->language)));
            native = utils::strings::replace(native, "{2}", utils::console::yellow(translate(defender->is_player() ? YOU_TEXT : defender->name.c_str(), this->language)));
            native = utils::strings::replace(native, "{3}", utils::console::yellow(utils::strings::itoa(damage)));
            native = utils::strings::replace(native, "{4}", translate(defender->is_player() ? YOU_TEXT : defender->name.c_str(), this->language));
            native = utils::strings::replace(native, "{5}", highlight_health_point(defender->max_health_point, previous_health_point));
            native = utils::strings::replace(native, "{6}", highlight_health_point(defender));
            if (p->send_string(string_info(false, FIGHT_LOG_DISPLAY_DELAY_MILLISECONDS, native))) {
                if (defender->is_dead()) {
                    native = translate(KNOCKDOWN_TEXT, this->language);
                    native = utils::strings::replace(native, "{1}", translate(defender->is_player() ? YOU_TEXT : defender->name.c_str(), this->language));
                    b = b && p->send_string(string_info(false, FIGHT_LOG_DISPLAY_DELAY_MILLISECONDS, native));
                    if (b && attacker->is_player() && defender->experience > 0) {
                        native = utils::console::green(translate(GOT_EXPERIENCE_TEXT, this->language));
                        native = utils::strings::replace(native, "{1}", utils::console::yellow(utils::strings::itoa(defender->experience)) + "\x1b[32m");
                        b = b && p->send_string(string_info(false, FIGHT_LOG_DISPLAY_DELAY_MILLISECONDS, native));
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
                                return p->send_string(string_info(false, FIGHT_LOG_DISPLAY_DELAY_MILLISECONDS, native));
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
    auto attackers1 = find_alive(characters1);
    auto attackers2 = find_alive(characters2);
    auto _attackers1 = std::vector<character *>(attackers1);
    auto _attackers2 = std::vector<character *>(attackers2);
    while (b && !is_fight_done(attackers1, attackers2)) {
        auto is_character1 = true;
        auto fastest = pickup_fastest_character(_attackers1, _attackers2, is_character1);
        if (fastest) {
            b = fighting(fastest, is_character1 ? attackers2 : attackers1);

            // All pending attackers are executed or the attacker who is pending for attack may already be knocked down in previous round,
            // so check if the attacker who is knocked down, if the attacker has been knocked out, then find a new character
            if ((_attackers1.empty() && _attackers2.empty()) || (is_all_knock_down(_attackers1) && is_all_knock_down(_attackers2))) {
                _attackers1 = find_alive(attackers1);
                _attackers2 = find_alive(attackers2);
            }
        } else {
            break;
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


