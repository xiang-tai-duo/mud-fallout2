//
// Created by 黄元镭 on 2023/11/1.
//

#include "maze.h"
#include "macros.h"
#include "utils.hpp"

enum DIRECTION : int {
    DOWN = 1,
    RIGHT = 2,
    LEFT = 4,
    UP = 8,
};

// 检查一个坐标是否在迷宫的范围内
bool is_inside_bounds(struct MAZE_MAP *maze, int x, int y) {
    return x >= 0 && x < maze->width() && y >= 0 && y < maze->height();
}

// 检查一个方向是否可以前进
bool is_move_assignable(struct MAZE_MAP *maze, int x, int y, DIRECTION direction) {
    switch (direction) {
        case UP:
            // 上方有两个单元格的距离，并且上方第二个单元格是墙，允许前进，中间的墙到时候会摧毁
            return is_inside_bounds(maze, x, y - 2) && maze->grid[y - 2][x] == WALL;
        case DOWN:
            // 下方有两个单元格的距离，并且下方第二个单元格是路径，下一个单元格是墙
            return is_inside_bounds(maze, x, y + 2) && maze->grid[y + 2][x] == WALL;
        case LEFT:
            // 左方有两个单元格的距离，并且左方第二个单元格是路径，左一个单元格是墙
            return is_inside_bounds(maze, x - 2, y) && maze->grid[y][x - 2] == WALL;
        case RIGHT:
            // 右方有两个单元格的距离，并且右方第二个单元格是路径，右一个单元格是墙
            return is_inside_bounds(maze, x + 2, y) && maze->grid[y][x + 2] == WALL;
    }
}

// 在一个方向上前进一步
void move(struct MAZE_MAP *maze, int &x, int &y, DIRECTION &direction) {
    switch (direction) {
        case UP:
            // 上方前进一步，打通中间的墙壁
            maze->grid[y - 1][x] = PATH;
            y -= 2;
            break;
        case DOWN:
            // 下方前进一步，打通中间的墙壁
            maze->grid[y + 1][x] = PATH;
            y += 2;
            break;
        case LEFT:
            // 左方前进一步，打通中间的墙壁
            maze->grid[y][x - 1] = PATH;
            x -= 2;
            break;
        case RIGHT:
            // 右方前进一步，打通中间的墙壁
            maze->grid[y][x + 1] = PATH;
            x += 2;
            break;
    }

    // 将当前位置标记为路径
    maze->set_cell_type(x, y, CELL_TYPE::PATH);
}

MAZE_MAP *generate_maze(int width, int height) {

    // 创建一个迷宫对象
    auto maze = new MAZE_MAP();

    // 设置列数
    maze->set_width(width < MIN_MAZE_WIDTH ? MIN_MAZE_WIDTH : width);

    // 设置行数
    maze->set_height(height < MIN_MAZE_HEIGHT ? MIN_MAZE_HEIGHT : height);

    // 初始化网格为全是墙壁
    maze->grid = std::vector<std::vector<CELL_TYPE >>(height, std::vector<CELL_TYPE>(width, WALL));
    std::random_device random;
    std::mt19937 mt19937(random());

    auto max_width_value = (int) maze->width() - 1;
    auto max_height_value = (int) maze->height() - 1;

    // 随机选择一个起点的坐标
    auto start_x = 0;
    auto start_y = 0;

    // 保证起点的列坐标是奇数
    do {
        start_x = std::uniform_int_distribution<int>(1, max_width_value < 0 ? 0 : max_width_value)(mt19937);
    } while (start_x % 2 == 0);

    // 保证起点的行坐标是奇数
    do {
        start_y = std::uniform_int_distribution<int>(1, max_height_value < 0 ? 0 : max_height_value)(mt19937);
    } while (start_y % 2 == 0);

    // 设置起点坐标
    maze->entrance = {start_x, start_y};

    // 将起点标记为START
    maze->set_cell_type(start_x, start_y, CELL_TYPE::START);

    // 创建一个栈，用来存储路径
    std::stack<std::pair<int, int>> path;

    // 将起点入栈
    path.push(maze->entrance);

    // 当栈不为空时，循环
    while (!path.empty()) {

        // 获取栈顶元素的列坐标
        int x = path.top().first;

        // 获取栈顶元素的行坐标
        int y = path.top().second;

        // 创建一个向量，用来存储可以前进的方向
        std::vector<DIRECTION> directions;

        // 如果可以向上前进，将UP加入向量
        if (is_move_assignable(maze, x, y, DIRECTION::UP)) {
            directions.push_back(DIRECTION::UP);
        }

        // 如果可以向下前进，将DOWN加入向量
        if (is_move_assignable(maze, x, y, DIRECTION::DOWN)) {
            directions.push_back(DIRECTION::DOWN);
        }

        // 如果可以向左前进，将LEFT加入向量
        if (is_move_assignable(maze, x, y, DIRECTION::LEFT)) {
            directions.push_back(DIRECTION::LEFT);
        }

        // 如果可以向右前进，将RIGHT加入向量
        if (is_move_assignable(maze, x, y, DIRECTION::RIGHT)) {
            directions.push_back(DIRECTION::RIGHT);
        }

        // 如果没有可以前进的方向，说明到达了一个死胡同
        if (directions.empty()) {

            // 将栈顶元素出栈，回溯到上一个位置
            path.pop();
        } else {

            // 如果有可以前进的方向，随机选择一个方向的索引，在该方向上前进一步，并且摧毁前进路径中的所有墙
            move(maze, x, y, directions[std::uniform_int_distribution<int>(0, (int) directions.size() - 1)(mt19937)]);

            // 将新的位置入栈
            path.emplace(x, y);
        }
    }

    // 找出所有的可行动路径
    std::vector<std::pair<int, int>> cells;
    for (auto y = 0; y < maze->height(); y++) {
        for (auto x = 0; x < maze->width(); x++) {
            if (maze->cell_type(x, y) == CELL_TYPE::PATH) {
                cells.emplace_back(x, y);
            }
        }
    }

    // 将一个随机路径作为终点
    maze->exit = cells[std::uniform_int_distribution<int>(0, (int) cells.size() - 1)(mt19937)];

    // 将终点标记为END
    maze->set_cell_type(maze->exit.first, maze->exit.second, CELL_TYPE::END);

    // 返回生成的迷宫
    return maze;
}
