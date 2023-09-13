//
// Created by 黄元镭 on 2023/8/3.
//

#ifndef MUD_FALLOUT_ENGINE_H
#define MUD_FALLOUT_ENGINE_H

#include "userdef.hpp"

#ifndef LOCALIZATION
inline std::string translate(const std::string &source, const std::string &language) {
    UNREFERENCE(language);
    return native;
}
#endif

#include "session.h"
#include "character.h"
#include "sha1.h"
#include "macros.h"
#include <iostream>

#ifdef WINDOWS

#include "../../cpp-httplib/httplib.h"

#else

#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

#endif

#include <cstring>
#include <thread>
#include <utility>

#ifdef WINDOWS
#define LISTEN_PORT (23)
#else
#define LISTEN_PORT (23)
#endif

namespace mud_fallout2::engine {
    static nlohmann::ordered_json keywords;

    static bool is_shutdown = false;

    [[maybe_unused]] void launch();

    [[maybe_unused]] inline void load_keywords();

    [[maybe_unused]] inline std::shared_ptr<character> login(const std::string &, const std::string &);

    [[maybe_unused]] inline std::thread *invoke(int);
}

#endif //MUD_FALLOUT_ENGINE_H
