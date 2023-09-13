//
// Created by 黄元镭 on 2023/9/14.
//

#include "stages.h"

stage stage::singleton;

stage::stage() {
    utils::console::trace("Initializing stages...");
    for (auto &file: utils::directory::get_files(STAGE_DIRECTORY_NAME)) {
        if (file.ends_with(".json")) {
            auto ifstream = std::ifstream(file);
            if (ifstream.is_open()) {
                std::string content((std::istreambuf_iterator<char>(ifstream)),
                                    (std::istreambuf_iterator<char>()));
                auto script = nlohmann::ordered_json::parse(content);
                stage::compile(script);
                this->m_stages.emplace_back(script);
                ifstream.close();
                utils::console::trace(utils::strings::format("%s was loaded. %s", file.c_str(), script.dump().c_str()));
            }
        }
    }
}

nlohmann::ordered_json stage::get(const std::string &id) {
    auto stage = nlohmann::ordered_json();
    for (auto &s: this->m_stages) {
        if (PROPERTY_ID(s) == id) {
            stage = s;
            break;
        }
    }
    if (stage.empty()) {
        stage = this->entrance();
    }
    return stage;
}

nlohmann::ordered_json stage::entrance() {
    auto stage = nlohmann::ordered_json();
    for (auto &s: this->m_stages) {
        if (PROPERTY_ENTRANCE(s)) {
            stage = s;
            break;
        }
    }
    if (stage.empty()) {
        utils::console::critical("entrance stage is missing.");
    }
    return stage;
}

void stage::compile(nlohmann::ordered_json &script, const nlohmann::ordered_json &current_path) {
    script[PROPERTY_NAME_PARENT_NODE] = current_path;
    auto is_modified = true;
    while (is_modified) {
        is_modified = false;
        for (auto it = script.begin(); it != script.end() && !is_modified; it++) {
            auto value = it.value();
            if (it.key() == PROPERTY_NAME_EVENTS) {
                // { "events": ["I want to go back to the village.", "Is there any news recently?", "It's nothing."] }
                // { "events": "I want to go back to the village." }
                if (value.is_array() || value.is_string()) {
                    auto events = nlohmann::ordered_json();
                    if (value.is_array()) {
                        for (auto e = it.value().begin(); e != it.value().end(); e++) {
                            if (e->is_string()) {
                                events[e.value().get<std::string>()] = nlohmann::ordered_json();
                            }
                        }
                    } else {
                        events[value.get<std::string>()] = nlohmann::ordered_json();
                    }
                    value = events;
                    is_modified = true;
                } else if (value.is_null()) {
                    throw std::invalid_argument("Not support for null");
                } else if (!value.is_object()) {
                    throw std::invalid_argument(utils::strings::format("Not support for type: %s", value.type_name()));
                }
            } else if (it.key() == PROPERTY_NAME_RESPONSE) {
                if (value.is_string()) {
                    auto result = nlohmann::ordered_json();
                    result.emplace_back(it.value().get<std::string>());
                    value = result;
                    is_modified = true;
                }
            }

            // "Talk to the natives": {}
            // "Talk to the natives": null
            if ((value.is_object() && !value.contains(PROPERTY_NAME_PARENT_NODE)) || value.is_null()) {
                if (value.is_null()) {
                    value = nlohmann::ordered_json();
                }
                value[PROPERTY_NAME_PARENT_NODE] = current_path;
                is_modified = true;
            }
            if (is_modified) {
                script[it.key()] = value;
            }
        }
    }

    for (auto it = script.begin(); it != script.end(); it++) {
        if (it->is_object()) {
            auto path = current_path;
            path.emplace_back(it.key());
            stage::compile(*it, path);
        }
    }
}

void stage::compile(nlohmann::ordered_json &script) {
    stage::compile(script, nlohmann::ordered_json::array());
}