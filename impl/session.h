//
// Created by 黄元镭 on 2023/9/18.
//

#ifndef MUD_FALLOUT2_SESSION_H
#define MUD_FALLOUT2_SESSION_H

#include "character.h"
#include <string>
#include <mutex>

#ifndef SOCKET_ERROR
#define SOCKET_ERROR (-1)
#endif

#define TERMINAL_UNKNOWN                        0
#define TERMINAL_WINDOWS                        1
#define TERMINAL_MACOS                          2

#define MESSAGE_DELAY_MILLISECONDS              1000

struct string_info {
public:
    bool translate;
    int delay;
    std::vector<std::string> format;

    string_info();

    string_info(const std::string &);

    string_info(const std::vector<std::string> &);

    string_info(bool, const std::string &);

    string_info(int, const std::string &);

    string_info(bool, int, const std::string &);

    string_info(bool, int, const std::vector<std::string> &);

    void init(bool, int, const std::string &);

    void init(bool, int, const std::vector<std::string> &);
};

class session {
public:
    explicit session(int);

    int socket;
    int terminal_type;
    std::string language;
    character *player;
    std::string buffer;

    [[maybe_unused]] inline std::string read();

    [[maybe_unused]] inline std::string read(int);

    [[maybe_unused]] inline std::string read_line();

    [[maybe_unused]] inline std::string read_line(int);

    [[maybe_unused]] inline bool read_line(std::string &);

    [[maybe_unused]] inline bool clear_buffer();

    [[maybe_unused]] [[nodiscard]] inline std::string replace_variable(const std::string &) const;

    [[maybe_unused]] inline bool send_string(string_info, ...);

    [[maybe_unused]] inline bool send_stage_description();

    [[maybe_unused]] inline bool send_new_line();

    [[maybe_unused]] inline void send_events();

    [[maybe_unused]] inline bool raise_event(const std::string &);

    [[maybe_unused]] inline bool send_status(const std::string &);

    [[maybe_unused]] inline bool send_rest(const std::string &);

    [[maybe_unused]] inline bool raise_encounter();

    [[maybe_unused]] inline bool send_response();

    [[maybe_unused]] inline std::vector<character *> generate_random_monsters() const;

    [[maybe_unused]] inline bool fight(std::vector<character *>, std::vector<character *>) const;

    [[maybe_unused]] inline bool fight(const std::vector<character *> &);

    [[maybe_unused]] static inline character *login(const std::string &, const std::string &);

    [[maybe_unused]] static inline std::string highlight_keywords(const std::string &);

    [[maybe_unused]] static inline std::string highlight_health_point(int, int);

    [[maybe_unused]] static inline std::string highlight_health_point(const character *);

protected:
    void init();

    std::mutex *socket_mutex;
};


#endif //MUD_FALLOUT2_SESSION_H
