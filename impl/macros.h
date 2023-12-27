//
// Created by 黄元镭 on 2023/9/11.
//

#ifndef MUD_FALLOUT_MACROS_H
#define MUD_FALLOUT_MACROS_H

#define CHOOSE_CONSOLE_ENCODING_TEXT    std::string("Select terminal")
#define MENU_ITEM_NAME_GBK              std::string("1. Windows")
#define MENU_ITEM_NAME_UTF8             std::string("2. macOS")
#define MENU_ITEM_NAME_WEB              std::string("3. Web")
#define COLOR_CONTROL_WORD              "\x1b["
#define COLOR_DEFAULT                   COLOR_CONTROL_WORD "0m"
#define UNREFERENCED(x) (x = x)

#ifndef SOCKET_ERROR
#define SOCKET_ERROR (-1)
#endif

#define WEBSOCKET_PORT                          9002

#define TERMINAL_UNKNOWN                        0
#define TERMINAL_WINDOWS                        1
#define TERMINAL_MACOS                          2
#define TERMINAL_WEB                            3

#define MESSAGE_DELAY_MILLISECONDS              1000

#define STATUS_CODE_OK                          200
#define STATUS_CODE_UNAUTHORIZED                401
#define STATUS_CODE_NOT_FOUND                   404
#define STATUS_CODE_NOT_ACCEPT                  406

#define JSON_KEY_USER_NAME                      "userName"
#define JSON_KEY_PASSWORD                       "password"
#define JSON_KEY_TEXT                           "text"
#define JSON_KEY_TEXT_TYPE                      "textType"
#define JSON_KEY_OPTION                         "option"
#define JSON_KEY_IMAGE                          "image"
#define JSON_KEY_MAZE                           "maze"
#define JSON_KEY_ENTRANCE                       "entrance"
#define JSON_KEY_EXIT                           "exit"
#define JSON_KEY_MAP                            "map"
#define JSON_KEY_X                              "x"
#define JSON_KEY_Y                              "y"
#define JSON_KEY_INDEX                          "index"
#define JSON_KEY_NAME                           "name"
#define JSON_KEY_STATUS_CODE                    "statusCode"
#define JSON_KEY_STATUS                         "status"
#define JSON_KEY_NAME                           "name"
#define JSON_KEY_LEVEL                          "level"
#define JSON_KEY_HEALTH_POINT                   "health_point"
#define JSON_KEY_MAX_HEALTH_POINT               "max_health_point"
#define JSON_KEY_POWER                          "power"
#define JSON_KEY_DEFENSIVE                      "defensive"
#define JSON_KEY_AGILITY                        "agility"
#define JSON_KEY_WEAPON                         "weapon"
#define JSON_KEY_ARMOR                          "armor"
#define JSON_KEY_EXPERIENCE                     "experience"

#define TEXT_TYPE_TEXT                          "text"
#define TEXT_TYPE_INLINE_TEXT                   "inline_text"
#define TEXT_TYPE_OPTIONS                       "options"

#define STAGE_DIRECTORY_NAME                    "Stage"

#define FLAG_PREFIX                            "FLAG"

#define PROPERTY_NAME_STAGE_ID                  "stage_id"
#define PROPERTY_NAME_NAME                      "name"
#define PROPERTY_NAME_ROLE                      "role"
#define PROPERTY_NAME_PASSWORD_HASH             "password_hash"
#define PROPERTY_NAME_ITEMS                     "items"
#define PROPERTY_NAME_MONSTERS                  "monsters"
#define PROPERTY_NAME_ENTRANCE                  "entrance"
#define PROPERTY_NAME_MAZE                      "maze"
#define PROPERTY_NAME_FLOORS                    "floors"
#define PROPERTY_NAME_NAME                      "name"
#define PROPERTY_NAME_LEVEL                     "level"
#define PROPERTY_NAME_HEALTH_POINT              "health_point"
#define PROPERTY_NAME_MAX_HEALTH_POINT          "max_health_point"
#define PROPERTY_NAME_ENERGY_POINT              "energy_point"
#define PROPERTY_NAME_MAX_ENERGY_POINT          "max_energy_point"
#define PROPERTY_NAME_POWER                     "power"
#define PROPERTY_NAME_DEFENSIVE                 "defensive"
#define PROPERTY_NAME_AGILITY                   "agility"
#define PROPERTY_NAME_EXPERIENCE                "experience"
#define PROPERTY_NAME_CURRENT_STAGE_EVENT_NAME  "current_stage_event_name"
#define PROPERTY_NAME_WELCOME                   "welcome"
#define PROPERTY_NAME_IMAGE                     "image"
#define PROPERTY_NAME_GOT                       "got"
#define PROPERTY_NAME_LOST                      "lost"
#define PROPERTY_NAME_MUST                      "must"
#define PROPERTY_NAME_DENIED                    "denied"
#define PROPERTY_NAME_NEXT_ACTION               "next_action"
#define PROPERTY_NAME_DURATION                  "duration"
#define PROPERTY_NAME_MAX_MONSTER_COUNT         "max_monster_count"

#define MAX_RECEIVE_BUFFER_SIZE                 1000
#define POLLING_MILLISECONDS                    1000
#define ENCOUNTER_POLLING_MILLISECONDS          1000
#define WEBSOCKET_DELAY_MILLISECONDS            500
#define REST_DELAY_MILLISECONDS                 1000
#define MOVE_DELAY_MILLISECONDS                 200
#define STATUS_DELAY_MILLISECONDS               100
#define ENCOUNTER_ODDS                          30
#define MAX_ENCOUNTER_MONSTER                   6
#define MACRO_PLAYER_NAME                       "{PLAYER_NAME}"

#define MIN_MAZE_WIDTH                          (5)
#define MIN_MAZE_HEIGHT                         (5)
#define MINI_MAP_RADIUS                         (5)

#ifdef _MAC
#define INFINITE                                (-1)
#endif

// 注意，在Windows中，中文编译了后总是以GBK编码存放在std::string中
#define WELCOME_TEXT                        "已连接到辐射2终端"
#define EVENTS_LIST_TEXT                    "当前可以使用的命令"
#define WRONG_USER_NAME_TEXT                "你站在避难所门口等了很久，突然警报声响起，一群全副武装的避难所安保人员出来把你赶走了"
#define GAME_OVER_TEXT1                     "你在一次战斗中不幸遇难，沙尘暴沙漠将你的残骸淹没。"
#define GAME_OVER_TEXT2                     "由于缺少水净化芯片来净化水源，阿罗由的人们决定在下一次辐射风暴之前撤离这个地方。"
#define MAZE_FLOOR_PROGRESS_TEXT            "正在调查地下城，进度：%s%%"
#define ENCOUNTER_TEXT                      "你遇到了："
#define ATTACK_ROUND_TEXT                   "{1}发起了攻击，对{2}造成了{3}点伤害，({4}的生命值：{5} -> {6})。"
#define KNOCKDOWN_TEXT                      "{1}被打倒了。"
#define YOU_TEXT                            "你"
#define GOT_EXPERIENCE_TEXT                 "你获得了{1}点经验值。"
#define LEVEL_UP_TEXT                       "你升级了！"
#define ABILITY_UP_TEXT                     "你的{1}提高了{2}点，{3} -> {4}。"
#define ABILITY_NAME_MAX_HEALTH_POINT_TEXT  "最大生命值"
#define ABILITY_NAME_POWER_TEXT             "力量"
#define ABILITY_NAME_DEFENSIVE_TEXT         "防御"
#define ABILITY_NAME_AGILITY_TEXT           "敏捷"
#define MENU_ITEM_NAME_REST                 "休息"

#endif //MUD_FALLOUT_MACROS_H
