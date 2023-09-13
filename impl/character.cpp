//
// Created by 黄元镭 on 2023/7/31.
//
#include "character.h"

character::character() {
    this->init();
}

character::character(const character *source) {
    this->name = source->name;
    this->role = source->role;
    this->password_hash = source->password_hash;
    this->level = source->level;
    this->stage = source->stage;
    this->health_point = source->health_point;
    this->max_health_point = source->max_health_point;
    this->energy_point = source->energy_point;
    this->power = source->power;
    this->defensive = source->defensive;
    this->agility = source->agility;
    this->health_point_recovery_rate = source->health_point_recovery_rate;
    this->experience = source->experience;
    this->events = source->events;
}

void character::init() {
    this->name.clear();
    this->role.clear();
    this->password_hash.clear();
    this->level = 1;
    this->health_point = 100;
    this->max_health_point = 100;
    this->energy_point = 0;
    this->power = 10;
    this->defensive = 3;
    this->agility = 3;
    this->health_point_recovery_rate = 2;
    this->experience = 0;
}

character *character::create() {
    auto p = new character();
    p->name = utils::strings::random();
    p->level = 1;
    p->health_point = utils::math::random(10, 20);
    p->max_health_point = p->health_point;
    p->power = utils::math::random(5, 10);
    p->defensive = utils::math::random(1, 3);
    p->agility = utils::math::random(1, 3);
    return p;
}

character *character::create(const utils::ordered_json &node) {
    auto p = new character();
    p->name = PROPERTY_NAME(node);
    p->level = PROPERTY_LEVEL(node);
    p->health_point = PROPERTY_HEALTH_POINT(node);
    p->max_health_point = PROPERTY_MAX_HEALTH_POINT(node);
    p->power = PROPERTY_POWER(node);
    p->defensive = PROPERTY_DEFENSIVE(node);
    p->agility = PROPERTY_AGILITY(node);
    return p;
}

character *character::create(const std::string &name, const std::string &password_hash, const utils::ordered_json &stage) {
    auto player = new character();
    player->name = name;
    player->role = ROLE_PLAYER;
    player->password_hash = password_hash;
    player->stage = stage;
    player->execute("");
    return player;
}

void character::add_health_point(int value) {
    this->health_point += value;
    if (this->health_point < 0) {
        this->health_point = 0;
    } else if (this->health_point > this->max_health_point) {
        this->health_point = this->max_health_point;
    }
    this->save();
}

void character::add_health_point() {
    this->add_health_point(this->health_point_recovery_rate);
}

bool character::is_dead() const {
    return this->health_point <= 0;
}

character *character::load(const utils::ordered_json &json) {
    auto character = character::create();
    character->name = utils::json::get_string(json, "name");
    character->role = utils::json::get_string(json, "role");
    character->password_hash = utils::json::get_string(json, "password_hash");
    character->flags = utils::json::get_array(json, "flags");
    character->health_point = utils::json::get_integer(json, "health_point");
    character->max_health_point = utils::json::get_integer(json, "max_health_point");
    character->energy_point = utils::json::get_integer(json, "energy_point");
    character->power = utils::json::get_integer(json, "power");
    character->defensive = utils::json::get_integer(json, "defensive");
    character->agility = utils::json::get_integer(json, "agility");
    character->stage = stages.get(utils::json::get_string(json, "stage"));
    character->events = character->stage;
    character->execute("");
    return character;
}

void character::save() {
    if (!this->password_hash.empty()) {
        auto json = utils::ordered_json();
        json["name"] = this->name;
        json["role"] = this->role;
        json["password_hash"] = this->password_hash;
        json["flags"] = utils::ordered_json::array();
        for (auto &flag: this->flags) {
            json["flags"].push_back(flag);
        }
        json["health_point"] = this->health_point;
        json["max_health_point"] = this->max_health_point;
        json["energy_point"] = this->energy_point;
        json["power"] = this->power;
        json["defensive"] = this->defensive;
        json["agility"] = this->agility;
        json["stage"] = PROPERTY_ID(this->stage);
        std::filesystem::create_directories(SAVE_DIRECTORY_NAME);
        std::ofstream file(utils::strings::format("%s/%s.json", SAVE_DIRECTORY_NAME, this->name.c_str()));
        if (file.is_open()) {
            file << json.dump(2);
            file.close();
        }
    }
}

utils::ordered_json character::get_available_events(const utils::ordered_json &node) {
    auto available_events = utils::ordered_json();
    auto events = PROPERTY_EVENTS(node);
    if (events.empty()) {
        auto parent_node = utils::json::get_strings(this->events, PROPERTY_NAME_PARENT_NODE);
        if (!parent_node.empty()) {
            events = character::find_event(this->stage, *(parent_node.end() - 1));
        }
    }
    if (events.empty()) {
        throw std::invalid_argument(utils::strings::format("Missing \"events\" element in:\n%s", node.dump().c_str()));
    }
    for (auto it = events.begin(); it != events.end(); it++) {
        if (this->is_usable(it.key(), it.value())) {
            available_events[it.key()] = it.value();
            available_events.generate_dump();
        }
    }
    return available_events;
}

utils::ordered_json character::get_available_events() {
    return this->get_available_events(this->events);
}

utils::ordered_json character::execute_event(const utils::ordered_json &node, const std::string &name) {
    auto response = utils::ordered_json();
    auto events = this->get_available_events(node);
    if (!events.empty()) {
        for (auto it = events.begin(); it != events.end(); it++) {
            if (it.key() == name) {
                response = it.value();
                break;
            }
        }
    }
    return response;
}

std::vector<std::string> character::get_response_text() {
    auto response = std::vector<std::string>();
    if (this->events.is_object()) {
        if (this->events.contains(PROPERTY_NAME_RESPONSE)) {
            response = PROPERTY_RESPONSE(this->events);
        } else {
            auto parent_node = utils::json::get_strings(this->events, PROPERTY_NAME_PARENT_NODE);
            if (!parent_node.empty()) {
                auto key = *(parent_node.end() - 1);
                auto event = character::find_event(this->stage, key);
                if (event.contains(key)) {
                    response = PROPERTY_RESPONSE(event[key]);
                }
            }
        }
    }
    return response;
}

inline utils::ordered_json character::find_event(const utils::ordered_json &node, const std::string &key) {
    auto result = utils::ordered_json();
    if (node.contains(key) && !node[key].empty()) {
        result = node[key];
    } else if (!result.contains(PROPERTY_NAME_RESPONSE) && node.is_object()) {
        for (const auto &it: node) {
            result = character::find_event(it, key);
            if (!result.empty()) {
                break;
            }
        }
    }
    return result;
}

utils::ordered_json character::execute(const std::string &event_name) {
    auto response = event_name.empty() ? this->stage : this->execute_event(this->events, event_name);
    if (!response.empty()) {
        if (!response.contains(PROPERTY_NAME_EVENTS)) {
            response = character::find_event(this->stage, event_name);
        }
        if (!response.empty()) {
            this->events = response;
            for (auto &got: PROPERTY_GOT(this->events)) {
                this->add_flag(got);
            }
            for (auto &lost: PROPERTY_LOST(this->events)) {
                this->remove_flag(lost);
            }
            auto next_stage = PROPERTY_NEXT_STAGE(this->events);
            if (!next_stage.empty()) {
                auto new_stage = stages.get(next_stage);
                if (!new_stage.empty()) {
                    this->stage = new_stage;
                    this->execute("");
                }
            }
        }
    }
    return response;
}

void character::add_flag(const std::string &flag) {
    if (!this->is_flag_exists(flag)) {
        this->flags.emplace_back(flag);
    }
}

void character::remove_flag(const std::string &flag) {
    auto b = false;
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

bool character::is_flag_exists(const std::string &flag) {
    auto b = false;
    for (auto &e: this->flags) {
        if (e == flag) {
            b = true;
            break;
        }
    }
    return b;
}

bool character::is_usable(const std::string &name, const utils::ordered_json &node) {
    auto b = name != PROPERTY_NAME_PARENT_NODE;
    if (b) {
        for (auto &necessary: PROPERTY_NECESSARY(node)) {
            if (!this->is_flag_exists(necessary)) {
                b = false;
                break;
            }
        }
        for (auto &denied: PROPERTY_DENIED(node)) {
            if (this->is_flag_exists(denied)) {
                b = false;
                break;
            }
        }
    }
    return b;
}

bool character::is_player() const {
    return this->role == ROLE_PLAYER;
}
