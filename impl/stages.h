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

#define STAGE_DIRECTORY_NAME            "Stage"
#define PROPERTY_NAME_EVENTS            "events"
#define PROPERTY_NAME_PARENT_NODE       "parent_node"
#define PROPERTY_NAME_RESPONSE          "response"
#define PROPERTY_NAME_NEXT_STAGE        "next_stage"

#define PROPERTY_ID(node)               utils::json::get_string(node, "id")
#define PROPERTY_NAME(node)             utils::json::get_string(node, "name")
#define PROPERTY_DESCRIPTION(node)      utils::json::get_strings(node, "description")
#define PROPERTY_MONSTERS(node)         utils::json::get_object(node, "monsters")
#define PROPERTY_EVENTS(node)           utils::json::get_object(node, PROPERTY_NAME_EVENTS)
#define PROPERTY_ENTRANCE(node)         utils::json::get_boolean(node, "entrance")
#define PROPERTY_PARENT_NODE(node)      utils::json::get_string(node, "parent_node")
#define PROPERTY_NAME(node)             utils::json::get_string(node, "name")
#define PROPERTY_LEVEL(node)            utils::json::get_integer(node, "level")
#define PROPERTY_HEALTH_POINT(node)     utils::json::get_integer(node, "health_point")
#define PROPERTY_MAX_HEALTH_POINT(node) utils::json::get_integer(node, "max_health_point")
#define PROPERTY_POWER(node)            utils::json::get_integer(node, "power")
#define PROPERTY_DEFENSIVE(node)        utils::json::get_integer(node, "defensive")
#define PROPERTY_AGILITY(node)          utils::json::get_integer(node, "agility")
#define PROPERTY_RESPONSE(node)         utils::json::get_strings(node, PROPERTY_NAME_RESPONSE)
#define PROPERTY_GOT(node)              utils::json::get_strings(node, "got")
#define PROPERTY_LOST(node)             utils::json::get_strings(node, "lost")
#define PROPERTY_NECESSARY(node)        utils::json::get_strings(node, "necessary")
#define PROPERTY_DENIED(node)           utils::json::get_strings(node, "denied")
#define PROPERTY_NEXT_STAGE(node)       utils::json::get_string(node, "next_stage")
#define PROPERTY_ENCOUNTER(node)        utils::json::get_string(node, "encounter")
#define PROPERTY_DURATION(node)         utils::json::get_integer(node, "duration")

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
