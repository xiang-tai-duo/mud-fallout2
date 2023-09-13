//
// Created by 黄元镭 on 2023/9/11.
//

#ifndef MUD_FALLOUT_MACROS_H
#define MUD_FALLOUT_MACROS_H

#define WELCOME_TEXT                    std::string("Welcome to Fallout2")
#define CHOOSE_CONSOLE_ENCODING_TEXT    std::string("Choose Terminal:")
#define MENU_ITEM_NAME_GBK              std::string("1. Windows")
#define MENU_ITEM_NAME_UTF8             std::string("2. macOS")
#define COLOR_CONTROL_WORD              "\x1b["

#define UNREFERENCED(x) (x = x)

#define EVENTS_LIST_TEXT          std::string("现阶段可以使用的命令")
#define REQUIRE_USER_NAME_TEXT    std::string("尊姓大名?")
#define REQUIRE_PASSCODE_TEXT     std::string("请输入口令")
#define WRONG_USER_NAME_TEXT      std::string("你站在避难所门口等了很久，突然警报声响起，一群全副武装的避难所安保人员出来把你赶走了")
#define GAME_OVER_TEXT            std::string("游戏结束")
#define MENU_ITEM_NAME_STATUS     std::string("x. 状态")
#define MENU_ITEM_NAME_REST       std::string("z. ZzzZzz")

#endif //MUD_FALLOUT_MACROS_H
