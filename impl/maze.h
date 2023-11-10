//
// Created by 黄元镭 on 2023/11/1.
//

#ifndef MUD_FALLOUT2_MAZE_H
#define MUD_FALLOUT2_MAZE_H

#include <vector>
#include <iostream>
#include <vector>
#include <stack>
#include <cstdlib>
#include <ctime>

// 定义迷宫的单元格类型
enum CELL_TYPE : int {
    NONE = -1,
    PATH = 0,                                       // 路径
    WALL = 1,                                       // 墙壁
    START = 2,                                      // 起点
    END = 4                                         // 终点
};

struct MAZE_MAP {
    std::vector<std::vector<CELL_TYPE>> grid;       // 网格
    std::pair<int, int> entrance;                   // 起点坐标
    std::pair<int, int> exit;                       // 终点坐标
    std::vector<std::pair<int, int>> treasures;     // 宝物

    CELL_TYPE cell_type(int x, int y) {
        auto type = CELL_TYPE::NONE;
        if (!this->grid.empty() &&
            x >= 0 && x < this->grid[0].size() &&
            y >= 0 && y < this->grid.size()) {
            type = this->grid[y][x];
        }
        return type;
    }

    void set_cell_type(int x, int y, CELL_TYPE cell_type) {
        if (!this->grid.empty() &&
            x >= 0 && x < this->grid[0].size() &&
            y >= 0 && y < this->grid.size()) {
            this->grid[y][x] = cell_type;
        }
    }

    int width() { return this->_width; }            // 列数
    int height() { return this->_height; }          // 行数

    void set_width(int width) { this->_width = width; }

    void set_height(int height) { this->_height = height; }

private:
    int _width;
    int _height;
};

MAZE_MAP *generate_maze(int, int);

#endif //MUD_FALLOUT2_MAZE_H
