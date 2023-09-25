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
#define NORMALIZE(x)                    utils::encoding::gbk_to_utf8(x)
#else
#define NORMALIZE(x)                    std::string(x)
#endif

// The text must be encoded in UTF8 before call send_string when terminal type is WINDOWS
// When you are building in windows, the hard coding string always encoded in GBK
// So make sure all hard coding string is encoded in utf8
// Do not use hard coding text in source code file
#define WELCOME_TEXT                        NORMALIZE("已连接到辐射2终端")
#define EVENTS_LIST_TEXT                    NORMALIZE("当前可以使用的命令")
#define REQUIRE_USER_NAME_TEXT              NORMALIZE("尊姓大名?")
#define REQUIRE_PASSCODE_TEXT               NORMALIZE("请输入口令")
#define WRONG_USER_NAME_TEXT                NORMALIZE("你站在避难所门口等了很久，突然警报声响起，一群全副武装的避难所安保人员出来把你赶走了")
#define GAME_OVER_TEXT                      NORMALIZE("与避难所的连线已中断。")
#define ENCOUNTER_TEXT                      NORMALIZE("你遇到了：")
#define ATTACK_ROUND_TEXT                   NORMALIZE("{1}发起了攻击，对{2}造成了{3}点伤害，({4}的生命值：{5} -> {6})。")
#define KNOCKDOWN_TEXT                      NORMALIZE("{1}被打倒了。")
#define YOU_TEXT                            NORMALIZE("你")
#define GOT_EXPERIENCE_TEXT                 NORMALIZE("你获得了{1}点经验值。")
#define LEVEL_UP_TEXT                       NORMALIZE("你升级了！")
#define ABILITY_UP_TEXT                     NORMALIZE("你的{1}提高了{2}点，{3} -> {4}。")
#define ABILITY_NAME_MAX_HEALTH_POINT_TEXT  NORMALIZE("最大生命值")
#define ABILITY_NAME_POWER_TEXT             NORMALIZE("力量")
#define ABILITY_NAME_DEFENSIVE_TEXT         NORMALIZE("防御")
#define ABILITY_NAME_AGILITY_TEXT           NORMALIZE("敏捷")
#define MENU_ITEM_NAME_STATUS           NORMALIZE("x. 状态")
#define MENU_ITEM_NAME_REST             NORMALIZE("z. ZzzZzz")

#endif //MUD_FALLOUT_MACROS_H
