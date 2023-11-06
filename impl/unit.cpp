//
// Created by 黄元镭 on 2023/7/31.
//
#include "unit.h"
#include "macros.h"

unit::unit() {
    this->init();
}

void unit::init() {
    this->name = utils::strings::random();
    this->role = ROLE_UNIT;
    this->password_hash = utils::strings::random();
    this->level = 1;
    this->health_point = 1;
    this->max_health_point = this->health_point;
    this->energy_point = 0;
    this->power = 1;
    this->defensive = 1;
    this->agility = 1;
    this->health_point_recovery_rate = 1;
    this->experience = 1;
}

void unit::add_health_point(int value) {
    this->health_point += value;
    if (this->health_point < 0) {
        this->health_point = 0;
    } else if (this->health_point > this->max_health_point) {
        this->health_point = this->max_health_point;
    }
}

void unit::add_health_point() {
    this->add_health_point(this->health_point_recovery_rate);
}

void unit::add_experience(int value) {
    this->experience += value;
    while (this->experience >= LEVEL_TABLE.get_experience(this->level + 1)) {
        this->level++;
        this->max_health_point += utils::math::random(1, 6);
        this->power += utils::math::random(1, 3);
        this->defensive += utils::math::random(1, 2);
        this->agility += utils::math::random(1, 2);
    }
}

bool unit::is_dead() const {
    return this->health_point <= 0;
}

unit *unit::load(const nlohmann::ordered_json &json) {
    auto p = new unit();
    p->name = utils::json::get_string(json, PROPERTY_NAME_NAME);
    p->role = utils::json::get_string(json, PROPERTY_NAME_ROLE);
    p->password_hash = utils::json::get_string(json, PROPERTY_NAME_PASSWORD_HASH);
    p->flags = utils::json::get_array(json, PROPERTY_NAME_FLAGS);
    p->level = utils::json::get_integer(json, PROPERTY_NAME_LEVEL);
    p->health_point = utils::json::get_integer(json, PROPERTY_NAME_HEALTH_POINT);
    p->max_health_point = utils::json::get_integer(json, PROPERTY_NAME_MAX_HEALTH_POINT);
    p->energy_point = utils::json::get_integer(json, PROPERTY_NAME_ENERGY_POINT);
    p->max_energy_point = utils::json::get_integer(json, PROPERTY_NAME_MAX_ENERGY_POINT);
    p->power = utils::json::get_integer(json, PROPERTY_NAME_POWER);
    p->defensive = utils::json::get_integer(json, PROPERTY_NAME_DEFENSIVE);
    p->agility = utils::json::get_integer(json, PROPERTY_NAME_AGILITY);
    p->experience = utils::json::get_integer(json, PROPERTY_NAME_EXPERIENCE);
    p->action = stage::singleton.get(utils::json::get_string(json, PROPERTY_NAME_ACTION));
    if (!p->execute(utils::json::get_string(json, PROPERTY_NAME_ACTION))) {
        p->action = stage::singleton.entrance();
    }
    return p;
}

void unit::save() {
    if (!this->password_hash.empty()) {
        auto json = nlohmann::ordered_json();
        json[PROPERTY_NAME_NAME] = this->name;
        json[PROPERTY_NAME_ROLE] = this->role;
        json[PROPERTY_NAME_PASSWORD_HASH] = this->password_hash;
        json[PROPERTY_NAME_FLAGS] = nlohmann::ordered_json::array();
        for (auto &flag: this->flags) {
            json[PROPERTY_NAME_FLAGS].push_back(flag);
        }
        json[PROPERTY_NAME_LEVEL] = this->level;
        json[PROPERTY_NAME_HEALTH_POINT] = this->health_point;
        json[PROPERTY_NAME_MAX_HEALTH_POINT] = this->max_health_point;
        json[PROPERTY_NAME_ENERGY_POINT] = this->energy_point;
        json[PROPERTY_NAME_MAX_ENERGY_POINT] = this->max_energy_point;
        json[PROPERTY_NAME_POWER] = this->power;
        json[PROPERTY_NAME_DEFENSIVE] = this->defensive;
        json[PROPERTY_NAME_AGILITY] = this->agility;
        json[PROPERTY_NAME_EXPERIENCE] = this->experience;
        json[PROPERTY_NAME_ACTION] = this->action.name;
        std::__fs::filesystem::create_directories(SAVE_DIRECTORY_NAME);
        std::ofstream file(utils::strings::format("%s/%s.json", SAVE_DIRECTORY_NAME, this->name.c_str()));
        if (file.is_open()) {
            file << json.dump(2);
            file.close();
        }
    }
}

std::vector<std::string> unit::options() {
    auto o = std::vector<std::string>();
    for (auto &option: this->action.options) {
        if (this->is_valid_option(option)) {
            o.emplace_back(option);
        }
    }
    return o;
}

bool unit::execute(const std::string &option_name) {
    auto b = false;
    auto a = stage::singleton.get(option_name);
    if (!a.name.empty()) {
        this->action = a;
        for (auto &got: this->action.got) {
            this->add_flag(got);
        }
        for (auto &lost: this->action.lost) {
            this->remove_flag(lost);
        }
        b = true;
    }
    return b;
}

void unit::add_flag(const std::string &flag) {
    if (!this->is_flag_exists(flag)) {
        this->flags.emplace_back(flag);
    }
}

void unit::remove_flag(const std::string &flag) {
    bool b;
    do {
        b = false;
        for (auto it = this->flags.begin(); it != this->flags.end(); it++) {
            if (*it == flag) {
                this->flags.erase(it);
                b = true;
                break;
            }
        }
    } while (b);
}

bool unit::is_flag_exists(const std::string &flag) {
    auto b = false;
    for (auto &e: this->flags) {
        if (e == flag) {
            b = true;
            break;
        }
    }
    return b;
}

bool unit::is_valid_option(const std::string &name) {
    auto a = stage::singleton.get(name);
    auto b = true;
    for (auto &necessary: a.necessary) {
        if (!this->is_flag_exists(necessary)) {
            b = false;
            break;
        }
    }
    for (auto &denied: a.denied) {
        if (this->is_flag_exists(denied)) {
            b = false;
            break;
        }
    }
    return b;
}

bool unit::is_player() const {
    return this->role == ROLE_PLAYER;
}
