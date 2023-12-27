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

struct STAGE_EVENT_ITEM {
    bool initialized{};
    std::string stage_id{};
    bool is_entrance_event{};               // 事件是否是入口事件
    std::string name{};
    bool entrance{};                        // 是否是新用户的入口场景
    struct MAZE {
        int duration;                       // 迷宫探索要使用的时间
        int max_monster_count;              // 最大遇敌数
    } maze{};
    std::vector<std::string> welcome;
    std::string image{};                    // 舞台图片，增加游戏体验
    std::vector<class unit *> monsters;
    std::vector<std::string> options;
    std::string next_action{};
    std::vector<std::string> must;
    std::vector<std::string> denied;
    std::vector<std::string> got;
    std::vector<std::string> lost;
    STAGE_EVENT_ITEM() {
        memset(&this->maze, 0, sizeof(this->maze));
    }
};

class stage {
public:
    stage();

    STAGE_EVENT_ITEM entrance();

    STAGE_EVENT_ITEM find_stage_event(const std::string &);

    STAGE_EVENT_ITEM find_stage_entrance_event(const std::string &id);

    static stage singleton;

protected:
    static STAGE_EVENT_ITEM new_stage_event_item(const nlohmann::ordered_json &);

    void compile(std::vector<STAGE_EVENT_ITEM> &, const nlohmann::ordered_json &, bool, const std::string &, const std::string &);

    static bool is_keyword(const std::string &);

    std::vector<STAGE_EVENT_ITEM> m_items;
};

#endif //MUD_FALLOUT_STAGE_H
