//
// Created by 黄元镭 on 2023/7/20.
//

#ifndef MUD_FALLOUT_CHARACTER_H
#define MUD_FALLOUT_CHARACTER_H

#include <string>
#include <random>
#include <memory>
#include "stages.h"
#include "utils.hpp"

#define MAX_LEVEL 999
#define SAVE_DIRECTORY_NAME "Save"
#define ROLE_PLAYER "player"

class character {
public:
    [[maybe_unused]] character();

    [[maybe_unused]] character(const character *);

    [[maybe_unused]] static character *create();

    [[maybe_unused]] static character *create(const utils::ordered_json &);

    [[maybe_unused]] static character *create(const std::string &, const std::string &, const utils::ordered_json &);

    [[maybe_unused]] static character *load(const utils::ordered_json &);

    [[maybe_unused]] void save();

    [[maybe_unused]] utils::ordered_json execute(const std::string &);

    [[maybe_unused]] utils::ordered_json get_available_events();

    [[maybe_unused]] std::vector<std::string> get_response_text();

    [[maybe_unused]] void add_health_point();

    [[maybe_unused]] void add_health_point(int);

    [[maybe_unused]] bool is_dead() const;

    [[maybe_unused]] bool is_player() const;

    [[maybe_unused]]void add_experience(int);

    std::string name;
    std::string role;
    std::string password_hash;
    std::vector<std::string> flags;
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
    utils::ordered_json stage;
    utils::ordered_json events;

protected:
    [[maybe_unused]] void init();

    [[maybe_unused]] utils::ordered_json get_available_events(const utils::ordered_json &node);

    [[maybe_unused]] utils::ordered_json execute_event(const utils::ordered_json &node, const std::string &name);

    [[maybe_unused]] static utils::ordered_json find_event(const utils::ordered_json &node, const std::string &key);

    [[maybe_unused]] void add_flag(const std::string &);

    [[maybe_unused]] void remove_flag(const std::string &);

    [[maybe_unused]] bool is_flag_exists(const std::string &);

    [[maybe_unused]] bool is_usable(const std::string &, const utils::ordered_json &);
};

static class level_table {
public:
    level_table() {
        this->m_table.push_back(0);
        for (auto i = 1; i <= MAX_LEVEL; i++) {
            this->m_table.push_back(this->m_table[i - 1] + (int)(i * 100 * 1.287653));
        }
    }

    int get_experience(int level) {
        return level <= this->m_table.size() ? this->m_table[level] : INT_MAX;
    }

protected:
    std::vector<int> m_table;
} LEVEL_TABLE;

#endif //MUD_FALLOUT_CHARACTER_H
