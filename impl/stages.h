//
// Created by 黄元镭 on 2023/7/20.
//

#ifndef MUD_FALLOUT_SCRIPT_H
#define MUD_FALLOUT_SCRIPT_H

#include <iostream>
#include <chrono>
#include <ctime>
#include <fstream>
#include <memory>
#include <string>
#include <vector>
#include "nlohmann/json.hpp"
#include "character.h"
#include "utils.hpp"

#define STAGE_DIRECTORY_NAME         "Stage"
#define PROPERTY_NAME_EVENTS         "events"
#define PROPERTY_NAME_PARENT_NODE    "parent_node"
#define PROPERTY_NAME_RESPONSE       "response"
#define PROPERTY_NAME_NEXT_STAGE     "next_stage"

#define PROPERTY_ID(x)               utils::json::get_string(x, "id")
#define PROPERTY_NAME(x)             utils::json::get_string(x, "name")
#define PROPERTY_DESCRIPTION(x)      utils::json::get_strings(x, "description")
#define PROPERTY_MONSTERS(x)         utils::json::get_object(x, "monsters")
#define PROPERTY_EVENTS(x)           utils::json::get_object(x, PROPERTY_NAME_EVENTS)
#define PROPERTY_ENTRANCE(x)         utils::json::get_boolean(x, "entrance")
#define PROPERTY_PARENT_NODE(x)      utils::json::get_string(x, "parent_node")
#define PROPERTY_NAME(x)             utils::json::get_string(x, "name")
#define PROPERTY_LEVEL(x)            utils::json::get_integer(x, "level")
#define PROPERTY_HEALTH_POINT(x)     utils::json::get_integer(x, "health_point")
#define PROPERTY_MAX_HEALTH_POINT(x) utils::json::get_integer(x, "max_health_point")
#define PROPERTY_POWER(x)            utils::json::get_integer(x, "power")
#define PROPERTY_DEFENSIVE(x)        utils::json::get_integer(x, "defensive")
#define PROPERTY_AGILITY(x)          utils::json::get_integer(x, "agility")
#define PROPERTY_EXPERIENCE(x)       utils::json::get_integer(x, "experience")
#define PROPERTY_RESPONSE(x)         utils::json::get_strings(x, PROPERTY_NAME_RESPONSE)
#define PROPERTY_GOT(x)              utils::json::get_strings(x, "got")
#define PROPERTY_LOST(x)             utils::json::get_strings(x, "lost")
#define PROPERTY_NECESSARY(x)        utils::json::get_strings(x, "necessary")
#define PROPERTY_DENIED(x)           utils::json::get_strings(x, "denied")
#define PROPERTY_NEXT_STAGE(x)       utils::json::get_string(x, "next_stage")
#define PROPERTY_ENCOUNTER(x)        utils::json::get_string(x, "encounter")
#define PROPERTY_DURATION(x)         utils::json::get_integer(x, "duration")

#define stages stage::singleton

class stage {
public:
    stage();

    nlohmann::ordered_json get(const std::string &);

    nlohmann::ordered_json entrance();

    static stage singleton;

protected:
    static void compile(nlohmann::ordered_json &, const nlohmann::ordered_json &);

    static void compile(nlohmann::ordered_json &);

    std::vector<nlohmann::ordered_json> m_stages;
};

#endif //MUD_FALLOUT_SCRIPT_H
