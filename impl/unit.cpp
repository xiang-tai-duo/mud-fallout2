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
    this->password_hash = std::string();
    this->level = 1;
    this->health_point = 16;
    this->max_health_point = this->health_point;
    this->energy_point = 0;
    this->power = 3;
    this->defensive = 1;
    this->agility = 1;
    this->health_point_recovery_rate = 1;
    this->experience = 0;
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
    p->items = utils::json::get_array(json, PROPERTY_NAME_ITEMS);
    p->level = utils::json::get_integer(json, PROPERTY_NAME_LEVEL);
    p->health_point = utils::json::get_integer(json, PROPERTY_NAME_HEALTH_POINT);
    p->max_health_point = utils::json::get_integer(json, PROPERTY_NAME_MAX_HEALTH_POINT);
    p->energy_point = utils::json::get_integer(json, PROPERTY_NAME_ENERGY_POINT);
    p->max_energy_point = utils::json::get_integer(json, PROPERTY_NAME_MAX_ENERGY_POINT);
    p->power = utils::json::get_integer(json, PROPERTY_NAME_POWER);
    p->defensive = utils::json::get_integer(json, PROPERTY_NAME_DEFENSIVE);
    p->agility = utils::json::get_integer(json, PROPERTY_NAME_AGILITY);
    p->experience = utils::json::get_integer(json, PROPERTY_NAME_EXPERIENCE);
    return p;
}

void unit::save() {
    if (!this->password_hash.empty()) {
        auto json = nlohmann::ordered_json();
        json[PROPERTY_NAME_NAME] = UTF8(this->name);
        json[PROPERTY_NAME_ROLE] = UTF8(this->role);
        json[PROPERTY_NAME_PASSWORD_HASH] = UTF8(this->password_hash);
        json[PROPERTY_NAME_ITEMS] = nlohmann::ordered_json::array();
        for (auto &item: this->items) {
            json[PROPERTY_NAME_ITEMS].push_back(UTF8(item));
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
        json[PROPERTY_NAME_CURRENT_STAGE_EVENT_NAME] = UTF8(this->current_stage_event.name);
        std::filesystem::create_directories(SAVE_DIRECTORY_NAME);
        std::ofstream file(utils::strings::format("%s/%s.json", SAVE_DIRECTORY_NAME, this->name.c_str()));
        if (file.is_open()) {
            file << json.dump(2);
            file.close();
        }
    }
}

std::vector<std::string> unit::get_options() {
    auto o = std::vector<std::string>();
    for (auto &option: this->current_stage_event.options) {
        if (this->is_valid_option(option)) {
            o.emplace_back(option);
        }
    }
    return o;
}

void unit::add_item(const std::string &item_name) {
    auto is_flag_item = utils::strings::starts_with(item_name, FLAG_PREFIX);
    if (is_flag_item) {
        if (!this->is_item_exists(item_name)) {
            this->items.emplace_back(item_name);
        }
    } else {
        this->items.emplace_back(item_name);
    }
}

void unit::delete_item(const std::string &item_name) {
    bool b;
    auto is_flag_item = utils::strings::starts_with(item_name, FLAG_PREFIX);
    do {
        b = false;
        for (auto it = this->items.begin(); it != this->items.end(); it++) {
            if (*it == item_name) {
                this->items.erase(it);
                b = true;
                if (!is_flag_item) {
                    break;
                }
            }
        }
    } while (b);
}

bool unit::is_item_exists(const std::string &flag) {
    auto b = false;
    for (auto &e: this->items) {
        if (e == flag) {
            b = true;
            break;
        }
    }
    return b;
}

bool unit::is_valid_option(const std::string &option) {
    auto a = stage::singleton.find_stage_event(option);
    auto b = a.initialized;
    if (b) {
        for (auto &must: a.must) {
            if (!this->is_item_exists(must)) {
                b = false;
                break;
            }
        }
        if (b) {
            for (auto &denied: a.denied) {
                if (this->is_item_exists(denied)) {
                    b = false;
                    break;
                }
            }
        }
    }
    return b;
}

bool unit::is_player() const {
    return this->role == ROLE_PLAYER;
}
