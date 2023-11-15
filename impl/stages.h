//
// Created by 黄元镭 on 2023/7/20.
//

#ifndef MUD_FALLOUT_STAGE_H
#define MUD_FALLOUT_STAGE_H

#include <iostream>
#include <chrono>
#include <ctime>
#include <fstream>
#include <memory>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include "utils.hpp"

class unit;

struct STAGE_ACTION {
    std::string stage_id;
    bool is_root;
    std::string name;
    bool entrance;
    struct MAZE {
        bool has_maze;
        int width;
        int height;
        int floors;
        int duration;
    } maze;
    std::vector<std::string> messages;
    std::vector<class unit *> monsters;
    std::vector<std::string> options;
    std::string next_action;
    std::vector<std::string> must;
    std::vector<std::string> denied;
    std::vector<std::string> got;
    std::vector<std::string> lost;
};

class stage {
public:
    stage();

    STAGE_ACTION entrance();

    STAGE_ACTION get(const std::string &);

    STAGE_ACTION stage_root(const std::string &);

    static stage singleton;

protected:
    static STAGE_ACTION init(const nlohmann::ordered_json &);

    void compile(std::vector<STAGE_ACTION> &, const nlohmann::ordered_json &, bool, const std::string &, const std::string &);

    static bool is_keyword(const std::string &);

    std::vector<STAGE_ACTION> m_actions;
};

#endif //MUD_FALLOUT_STAGE_H
