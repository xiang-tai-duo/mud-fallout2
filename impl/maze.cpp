//
// Created by 黄元镭 on 2023/11/1.
//

#include "maze.h"
#include <iostream>
#include <vector>
#include <random>
#include <chrono>

#define WALL        (-1)
#define ROAD        2

enum MAZE_WALL_EDGE : int {
    DOWN = 1,
    RIGHT = 2,
    LEFT = 4,
    UP = 8,
};

struct MAZE_WALL {
    int row, column;
    MAZE_WALL_EDGE direction;

    MAZE_WALL(int _row, int _column, MAZE_WALL_EDGE _direction) {
        row = _row;
        column = _column;
        direction = _direction;
    }
};

void get_surrounding_walls(std::vector<std::vector<int>> &map, std::vector<MAZE_WALL> &walls, int x, int y, int width, int height) {
    // @formatter:off
    if (x + 1 < width - 1 && map[x + 1][y] == WALL) { walls.emplace_back(x + 1, y, MAZE_WALL_EDGE::DOWN); }
    if (y + 1 < height - 1 && map[x][y + 1] == WALL) { walls.emplace_back(x, y + 1, MAZE_WALL_EDGE::RIGHT); }
    if (x - 1 > 1 && map[x - 1][y] == WALL) { walls.emplace_back(x - 1, y, MAZE_WALL_EDGE::UP); }
    if (y - 1 > 1 && map[x][y - 1] == WALL) { walls.emplace_back(x, y - 1, MAZE_WALL_EDGE::LEFT); }
    // @formatter:on
}

int create_maze(int start_x, int start_y, int width, int height) {
    std::vector<std::vector<int>> map;
    std::vector<MAZE_WALL> walls;
    auto actual_width = width < 3 ? 3 : width;
    auto actual_height = height < 3 ? 3 : height;
    for (auto y = 0; y < actual_height; y++) {
        std::vector<int> blocks;
        blocks.clear();
        for (auto x = 0; x < actual_width; x++) {
            blocks.emplace_back(WALL);
        }
        map.emplace_back(blocks);
    }

    // 定义起始点
    auto entrance_x = start_x < 1 || start_x >= width - 1 ? 1 : start_x;
    auto entrance_y = start_y < 1 || start_y >= height - 1 ? 1 : start_y;

    // 矿工位置
    auto position_x = entrance_x;
    auto position_y = entrance_y;

    // 起始点必定是可以行走，所以要设置为通路
    map[entrance_x][entrance_y] = ROAD;

    // 找出与当前位置相邻的墙
    get_surrounding_walls(map, walls, position_x, position_y, actual_width, actual_height);

    // 第一步压入两堵墙（起点右边和起点下面）进入循环
    while (!walls.empty()) {
        std::random_device rd;
        std::mt19937 gen(rd());

        // 随机选择一堵墙（生成0 ~ BlockSize-1之间的随机数，同时也是vector里墙的下标）
        auto max = (int) walls.size() - 1;
        auto random_value = std::uniform_int_distribution<int>(0, max < 0 ? 0 : max)(gen);
        MAZE_WALL target_wall = walls[random_value];

        // 矿工来到我们“选择的墙”这里，注意这里x和y是相反操作，所以会将矿工和墙放在同一个直线上
        position_x = target_wall.row;
        position_y = target_wall.column;

        // 根据当前选择的墙的方向进行后续操作
        // 此时，起始点 选择的墙 目标块 三块区域在同一直线上
        // 我们让矿工从“选择的墙”继续前进到“目标块”
        switch (target_wall.direction) {
            // @formatter:off
            case MAZE_WALL_EDGE::DOWN: if (position_x < width - 2) { position_x++; } break;
            case MAZE_WALL_EDGE::RIGHT: if (position_y < height - 2) { position_y++; } break;
            case MAZE_WALL_EDGE::LEFT: if (position_y > 1) { position_y--; } break;
            case MAZE_WALL_EDGE::UP: if (position_x > 1) { position_x--; } break;
            // @formatter:on
        }

        // 目标块如果是墙
        if (map[position_x][position_y] == WALL) {

            // 打通墙和目标块
            map[target_wall.row][target_wall.column] = ROAD;
            map[position_x][position_y] = ROAD;

            // 再次找出与矿工当前位置相邻的墙
            get_surrounding_walls(map, walls, position_x, position_y, actual_width, actual_height);
        }

        // 删除这堵墙(把用不了的墙删了，对于那些已经施工过了不必再施工了，同时也是确保我们能跳出循环)
        walls.erase(walls.begin() + random_value);
    }
    for (int i = 0; i < actual_width; i++) {
        for (int j = 0; j < actual_height; j++) {
            if (i == entrance_x && j == entrance_y) {
                printf("*");
            } else if (map[i][j] == ROAD) {
                printf(" ");
            } else {
                printf("#");
            }
        }
        printf("\n");
    }
    return 0;
}
