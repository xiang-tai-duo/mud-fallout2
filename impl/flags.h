//
// Created by 黄元镭 on 2023/9/7.
//

#ifndef MUD_FALLOUT_FLAGS_H
#define MUD_FALLOUT_FLAGS_H

#include <string>
#include <vector>

struct FLAGS {
    std::vector<std::string> must_exists;
    std::vector<std::string> must_not_exists;
};

#endif //MUD_FALLOUT_FLAGS_H
