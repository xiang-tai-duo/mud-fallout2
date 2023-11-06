//
// Created by 黄元镭 on 2023/9/14.
//

#include "stages.h"
#include "macros.h"
#include "unit.h"

stage stage::singleton;

stage::stage() {
    this->m_actions.clear();
    utils::console::trace("Initializing stages...");
    for (auto &file: utils::directory::get_files(STAGE_DIRECTORY_NAME)) {
        if (utils::strings::ends_with(file, ".json")) {
            auto ifstream = std::ifstream(file);
            if (ifstream.is_open()) {
                std::string content((std::istreambuf_iterator<char>(ifstream)),
                                    (std::istreambuf_iterator<char>()));
                ifstream.close();
                std::vector<STAGE_ACTION> actions;
                auto ordered_json = nlohmann::ordered_json::parse(content);
                this->compile(actions,
                              ordered_json,
                              true,
                              utils::json::get_string(ordered_json, PROPERTY_NAME_STAGE_ID),
                              utils::json::get_string(ordered_json, PROPERTY_NAME_NAME));
                for (auto i = (int) actions.size() - 1; i >= 0; i--) {
                    this->m_actions.push_back(actions.at(i));
                }
                utils::console::trace(utils::strings::format("%s was loaded.", file.c_str()));
            }
        }
    }

    auto entrance_count = 0;
    for (auto &action: this->m_actions) {
        if (action.entrance) {
            entrance_count++;
        }
        if (!action.next_action.empty()) {
            for (auto &e: this->m_actions) {
                if (e.stage_id == action.next_action) {
                    action.options = e.options;
                    break;
                }
            }
        }
    }
    if (entrance_count == 0) {
        utils::console::throw_("There should be at least one stage with an entrance property.");
    } else if (entrance_count > 1) {
        utils::console::throw_("Duplicate entrances were found.");
    }
}

STAGE_ACTION stage::get(const std::string &action_name) {
    auto action = STAGE_ACTION();
    for (auto &a: this->m_actions) {
        if (a.name == action_name) {
            action = a;
            break;
        }
    }
    return action;
}

STAGE_ACTION stage::stage_root(const std::string &id) {
    auto action = STAGE_ACTION();
    for (auto &a: this->m_actions) {
        if (a.stage_id == id && a.is_root) {
            action = a;
            break;
        }
    }
    return action;
}

STAGE_ACTION stage::entrance() {
    auto stage = STAGE_ACTION();
    for (auto &s: this->m_actions) {
        if (s.entrance) {
            stage = s;
            break;
        }
    }
    return stage;
}

bool stage::is_keyword(const std::string &key) {
    return key == PROPERTY_NAME_STAGE_ID ||
           key == PROPERTY_NAME_NAME ||
           key == PROPERTY_NAME_MONSTERS ||
           key == PROPERTY_NAME_ENTRANCE ||
           key == PROPERTY_NAME_NAME ||
           key == PROPERTY_NAME_LEVEL ||
           key == PROPERTY_NAME_HEALTH_POINT ||
           key == PROPERTY_NAME_MAX_HEALTH_POINT ||
           key == PROPERTY_NAME_POWER ||
           key == PROPERTY_NAME_DEFENSIVE ||
           key == PROPERTY_NAME_AGILITY ||
           key == PROPERTY_NAME_EXPERIENCE ||
           key == PROPERTY_NAME_MESSAGES ||
           key == PROPERTY_NAME_GOT ||
           key == PROPERTY_NAME_LOST ||
           key == PROPERTY_NAME_NECESSARY ||
           key == PROPERTY_NAME_DENIED ||
           key == PROPERTY_NAME_NEXT_ACTION ||
           key == PROPERTY_NAME_ENCOUNTER ||
           key == PROPERTY_NAME_DURATION;
}

STAGE_ACTION stage::init(const nlohmann::ordered_json &ordered_json) {
    STAGE_ACTION action;
    action.name = utils::json::get_string(ordered_json, PROPERTY_NAME_NAME);
    action.stage_id = utils::json::get_string(ordered_json, PROPERTY_NAME_STAGE_ID);
    action.entrance = utils::json::get_boolean(ordered_json, PROPERTY_NAME_ENTRANCE);
    action.encounter = utils::json::get_boolean(ordered_json, PROPERTY_NAME_ENCOUNTER);
    action.duration = utils::json::get_boolean(ordered_json, PROPERTY_NAME_DURATION);
    action.messages = utils::json::get_strings(ordered_json, PROPERTY_NAME_MESSAGES);
    action.next_action = utils::json::get_string(ordered_json, PROPERTY_NAME_NEXT_ACTION);
    action.necessary = utils::json::get_strings(ordered_json, PROPERTY_NAME_NECESSARY);
    action.denied = utils::json::get_strings(ordered_json, PROPERTY_NAME_DENIED);
    action.got = utils::json::get_strings(ordered_json, PROPERTY_NAME_GOT);
    action.lost = utils::json::get_strings(ordered_json, PROPERTY_NAME_LOST);
    auto monsters = utils::json::get_object(ordered_json, PROPERTY_NAME_MONSTERS);
    for (auto &it: monsters) {
        auto monster = new unit();
        monster->role = ROLE_MONSTER;
        monster->name = utils::json::get_string(it, PROPERTY_NAME_NAME);
        monster->health_point = utils::json::get_integer(it, PROPERTY_NAME_HEALTH_POINT);
        monster->max_health_point = utils::json::get_integer(it, PROPERTY_NAME_MAX_HEALTH_POINT);
        monster->power = utils::json::get_integer(it, PROPERTY_NAME_POWER);
        monster->defensive = utils::json::get_integer(it, PROPERTY_NAME_DEFENSIVE);
        monster->agility = utils::json::get_integer(it, PROPERTY_NAME_AGILITY);
        monster->experience = utils::json::get_integer(it, PROPERTY_NAME_EXPERIENCE);
        action.monsters.push_back(monster);
    }
    return action;
}

void stage::compile(std::vector<STAGE_ACTION> &actions, const nlohmann::ordered_json &ordered_json, bool is_root, const std::string &stage_id, const std::string &name) {
    auto action = stage::init(ordered_json);
    action.stage_id = stage_id;
    action.is_root = is_root;
    action.name = name;
    for (auto child = ordered_json.begin(); child != ordered_json.end(); child++) {
        if (!stage::is_keyword(child.key())) {
            action.options.emplace_back(child.key());
            stage::compile(actions, child.value(), false, stage_id, child.key());
        }
    }
    if (!ordered_json.empty()) {
        actions.push_back(action);
    }
}
