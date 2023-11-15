﻿//
// Created by 黄元镭 on 2023/7/20.
//

#ifndef MUD_FALLOUT_CHARACTER_H
#define MUD_FALLOUT_CHARACTER_H

#include <string>
#include <random>
#include <memory>
#include "stages.h"
#include "utils.hpp"
#include "maze.h"

#define MAX_LEVEL 999
#define SAVE_DIRECTORY_NAME "Save"
#define ROLE_PLAYER "player"
#define ROLE_UNIT "unit"
#define ROLE_MONSTER "monster"

class unit {
public:
    [[maybe_unused]] unit();

    [[maybe_unused]] static unit *load(const nlohmann::ordered_json &);

    [[maybe_unused]] void save();

    [[maybe_unused]] bool execute(const std::string &);

    [[maybe_unused]] void add_health_point();

    [[maybe_unused]] void add_health_point(int);

    [[maybe_unused]] bool is_dead() const;

    [[maybe_unused]] bool is_player() const;

    [[maybe_unused]] void add_experience(int);

    [[maybe_unused]] std::vector<std::string> options();

    std::string name;
    std::string role;
    std::string password_hash;
    std::vector<std::string> flags;
    STAGE_ACTION action;
    std::map<std::string, std::vector<struct MAZE_MAP *> *> all_maze_maps;
    int level{};
    int health_point{};
    int max_health_point{};
    int energy_point{};
    int max_energy_point{};
    int power{};
    int defensive{};
    int agility{};
    int health_point_recovery_rate{};
    int experience{};
    int maze_position_x{};
    int maze_position_y{};
    int maze_map_index{};
    std::string maze_name;

protected:
    [[maybe_unused]] void init();

    [[maybe_unused]] void add_flag(const std::string &);

    [[maybe_unused]] void remove_flag(const std::string &);

    [[maybe_unused]] bool is_flag_exists(const std::string &);

    [[maybe_unused]] bool is_valid_option(const std::string &option);
};

static class level_table {
public:
    level_table() {
        this->m_table.push_back(0);
        for (auto i = 1; i <= MAX_LEVEL; i++) {
            this->m_table.push_back(this->m_table[i - 1] + (int) (i * 100 * 1.287653));
        }
    }

    int get_experience(int level) {
        return level <= this->m_table.size() ? this->m_table[level] : INT_MAX;
    }

protected:
    std::vector<int> m_table;
} LEVEL_TABLE;

#endif //MUD_FALLOUT_CHARACTER_H
