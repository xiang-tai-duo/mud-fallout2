//
// Created by 黄元镭 on 2023/9/14.
//

#include "stages.h"
#include "macros.h"
#include "unit.h"

stage stage::singleton;

stage::stage() {
    this->m_items.clear();
    utils::console::trace("Initializing stages...");
    auto is_first_stage = true;
    for (auto &file: utils::directory::get_files(STAGE_DIRECTORY_NAME)) {
        if (utils::strings::ends_with(file, ".json")) {
            auto ifstream = std::ifstream(file);
            if (ifstream.is_open()) {
                std::string content((std::istreambuf_iterator<char>(ifstream)),
                                    (std::istreambuf_iterator<char>()));
                ifstream.close();
                std::vector<STAGE_EVENT_ITEM> items;
                auto ordered_json = nlohmann::ordered_json::parse(content);

                // json转为vector便于调试
                this->compile(items,
                              ordered_json,
                              true,
                              utils::json::get_string(ordered_json, PROPERTY_NAME_STAGE_ID),
                              utils::json::get_string(ordered_json, PROPERTY_NAME_NAME));
                for (auto i = 0; i < items.size(); i++) {
                    auto item = items.at(i);
                    if (is_first_stage) {
                        if (i == 0) {
                            utils::console::trace(item.name);
                        } else {
                            utils::console::trace("%d: %s", i, item.name.c_str());
                        }
                    }
                    this->m_items.insert(this->m_items.begin(), item);
                }
                utils::console::trace(utils::strings::format("%s was loaded.", file.c_str()));
                is_first_stage = false;
            }
        }
    }

    auto entrance_count = 0;
    for (auto &action: this->m_items) {
        if (action.is_entrance_stage) {
            entrance_count++;
        }
        if (!action.next_action.empty()) {
            for (auto &e: this->m_items) {
                if (e.stage_id == action.next_action) {
                    action.options = e.options;
                    break;
                }
            }
        }
    }
    if (entrance_count == 0) {
        utils::console::_throw("There should be at least one stage with an entrance property.");
    } else if (entrance_count > 1) {
        utils::console::_throw("Duplicate entrances were found.");
    }
}

STAGE_EVENT_ITEM stage::find_stage_event(const std::string &name) {
    auto item = STAGE_EVENT_ITEM();
    if (!name.empty()) {
        for (auto &a: this->m_items) {
            if (a.name == name) {
                item = a;
                break;
            }
        }
    }
    return item;
}

STAGE_EVENT_ITEM stage::find_stage_entrance_event(const std::string &id) {
    auto action = STAGE_EVENT_ITEM();
    for (auto &a: this->m_items) {
        if (a.stage_id == id && a.is_entrance_event) {
            action = a;
            break;
        }
    }
    return action;
}

STAGE_EVENT_ITEM stage::entrance() {
    auto stage = STAGE_EVENT_ITEM();
    for (auto &s: this->m_items) {
        if (s.is_entrance_stage) {
            stage = s;
            break;
        }
    }
    return stage;
}

// 纯英语单词和数字认为是一个节点关键字，而不是某一个事件或者可选项
bool stage::is_keyword(const std::string &key) {
    auto b = true;
    for (auto &c: key) {
        if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'))) {
            b = false;
            break;
        }
    }
    return b;
}

STAGE_EVENT_ITEM stage::new_stage_event_item(const nlohmann::ordered_json &ordered_json) {
    STAGE_EVENT_ITEM item;
    item.initialized = false;
    item.name = utils::json::get_string(ordered_json, PROPERTY_NAME_NAME);
    item.stage_id = utils::json::get_string(ordered_json, PROPERTY_NAME_STAGE_ID);
    item.is_entrance_stage = utils::json::get_boolean(ordered_json, PROPERTY_NAME_ENTRANCE);
    auto maze_json = utils::json::get_object(ordered_json, PROPERTY_NAME_MAZE);
    if (!maze_json.empty()) {
        item.maze.floors = utils::json::get_integer(maze_json, PROPERTY_NAME_FLOORS);
        item.maze.duration = utils::json::get_integer(maze_json, PROPERTY_NAME_DURATION);
        item.maze.initialized = true;
    }
    item.messages = utils::json::get_strings(ordered_json, PROPERTY_NAME_MESSAGES);
    item.next_action = utils::json::get_string(ordered_json, PROPERTY_NAME_NEXT_ACTION);
    item.must = utils::json::get_strings(ordered_json, PROPERTY_NAME_MUST);
    item.denied = utils::json::get_strings(ordered_json, PROPERTY_NAME_DENIED);
    item.got = utils::json::get_strings(ordered_json, PROPERTY_NAME_GOT);
    item.lost = utils::json::get_strings(ordered_json, PROPERTY_NAME_LOST);
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
        item.monsters.push_back(monster);
    }
    return item;
}

void stage::compile(std::vector<STAGE_EVENT_ITEM> &items, const nlohmann::ordered_json &ordered_json, bool is_entrance_event, const std::string &stage_id, const std::string &name) { // NOLINT(*-no-recursion)
    // 要先判断该节点是否是空节点，如果是空节点说明这个节点引用了一个其他的节点，那么不要添加这个节点
    // "[阿罗由神庙] 离开避难所": {}
    if (!ordered_json.empty()) {
        auto item = stage::new_stage_event_item(ordered_json);
        item.initialized = true;
        item.stage_id = stage_id;
        item.is_entrance_event = is_entrance_event;

        // 这里要注意由于根节点是没有key的，所以第一次调用的时候会先读取根节点的name属性
        // name: "阿罗由神庙"，作为key传进来
        item.name = name;
        for (auto child = ordered_json.begin(); child != ordered_json.end(); child++) {
            if (child->is_object() && !stage::is_keyword(child.key())) {
                item.options.emplace_back(GBK(child.key()));
            }
        }
        items.push_back(item);
        for (auto child = ordered_json.begin(); child != ordered_json.end(); child++) {
            auto key = GBK(child.key());
            if (child->is_object() && !stage::is_keyword(key)) {

                // 将object的key: "[阿罗由神庙] 尝试用锤子暴力破解面板" 作为name继续递归转换
                stage::compile(items, child.value(), false, stage_id, key);
            }
        }
    }
}
