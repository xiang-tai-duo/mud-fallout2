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

#ifdef WINDOWS
#define UTF8(x)                    utils::encoding::gbk_to_utf8(x)
#else
#define UTF8(x)                    std::string(x)
#endif

#define JSON_KEY_USER_NAME  "userName"
#define JSON_KEY_PASSWORD  "password"

#define STAGE_DIRECTORY_NAME             "Stage"
#define PROPERTY_NAME_STAGE_ID           "stage_id"
#define PROPERTY_NAME_NAME               "name"
#define PROPERTY_NAME_ROLE               "role"
#define PROPERTY_NAME_PASSWORD_HASH      "password_hash"
#define PROPERTY_NAME_FLAGS              "flags"
#define PROPERTY_NAME_MONSTERS           "monsters"
#define PROPERTY_NAME_ENTRANCE           "entrance"
#define PROPERTY_NAME_NAME               "name"
#define PROPERTY_NAME_LEVEL              "level"
#define PROPERTY_NAME_HEALTH_POINT       "health_point"
#define PROPERTY_NAME_MAX_HEALTH_POINT   "max_health_point"
#define PROPERTY_NAME_ENERGY_POINT       "energy_point"
#define PROPERTY_NAME_MAX_ENERGY_POINT   "max_energy_point"
#define PROPERTY_NAME_POWER              "power"
#define PROPERTY_NAME_DEFENSIVE          "defensive"
#define PROPERTY_NAME_AGILITY            "agility"
#define PROPERTY_NAME_EXPERIENCE         "experience"
#define PROPERTY_NAME_ACTION             "action"
#define PROPERTY_NAME_MESSAGES           "messages"
#define PROPERTY_NAME_GOT                "got"
#define PROPERTY_NAME_LOST               "lost"
#define PROPERTY_NAME_NECESSARY          "necessary"
#define PROPERTY_NAME_DENIED             "denied"
#define PROPERTY_NAME_NEXT_ACTION        "next_action"
#define PROPERTY_NAME_ENCOUNTER          "encounter"
#define PROPERTY_NAME_DURATION           "duration"

// The text must be encoded in UTF8 before call send_string when terminal type is WINDOWS
// When you are building in windows, the hard coding string always encoded in GBK
// So make sure all hard coding string is encoded in utf8
// Do not use hard coding text in source code file
#define WELCOME_TEXT                        UTF8("已连接到辐射2终端")
#define EVENTS_LIST_TEXT                    UTF8("当前可以使用的命令")
#define WRONG_USER_NAME_TEXT                UTF8("你站在避难所门口等了很久，突然警报声响起，一群全副武装的避难所安保人员出来把你赶走了")
#define GAME_OVER_TEXT                      UTF8("与避难所的连线已中断。")
#define ENCOUNTER_TEXT                      UTF8("你遇到了：")
#define ATTACK_ROUND_TEXT                   UTF8("{1}发起了攻击，对{2}造成了{3}点伤害，({4}的生命值：{5} -> {6})。")
#define KNOCKDOWN_TEXT                      UTF8("{1}被打倒了。")
#define YOU_TEXT                            UTF8("你")
#define GOT_EXPERIENCE_TEXT                 UTF8("你获得了{1}点经验值。")
#define LEVEL_UP_TEXT                       UTF8("你升级了！")
#define ABILITY_UP_TEXT                     UTF8("你的{1}提高了{2}点，{3} -> {4}。")
#define ABILITY_NAME_MAX_HEALTH_POINT_TEXT  UTF8("最大生命值")
#define ABILITY_NAME_POWER_TEXT             UTF8("力量")
#define ABILITY_NAME_DEFENSIVE_TEXT         UTF8("防御")
#define ABILITY_NAME_AGILITY_TEXT           UTF8("敏捷")
#define MENU_ITEM_NAME_REST                 UTF8("休息")

#endif //MUD_FALLOUT_MACROS_H
