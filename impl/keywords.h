//
// Created by 黄元镭 on 2023/10/31.
//

#ifndef MUD_FALLOUT2_KEYWORDS_H
#define MUD_FALLOUT2_KEYWORDS_H

#include <json.hpp>
#include <fstream>

#define KEYWORD_JSON_FILE_NAME "Config/keywords.json"

static nlohmann::ordered_json keywords;

inline void load_keywords() {
    auto file = std::ifstream(KEYWORD_JSON_FILE_NAME);
    if (file.is_open()) {
        std::string content((std::istreambuf_iterator<char>(file)),
                            (std::istreambuf_iterator<char>()));
        keywords = nlohmann::ordered_json::parse(content);
        file.close();
#ifdef WINDOWS
        auto gbk = nlohmann::ordered_json();
            for (auto it = keywords.begin(); it != keywords.end(); it++) {
                auto key = utils::encoding::utf8_to_gbk(it.key());
                auto value = utils::encoding::utf8_to_gbk(it.value().get<std::string>());
                gbk[key] = value;
            }
            keywords = gbk;
#endif
    }
}

#endif //MUD_FALLOUT2_KEYWORDS_H
