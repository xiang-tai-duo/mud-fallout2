//
// Created by 黄元镭 on 2023/9/18.
//

#ifndef MUD_FALLOUT2_SESSION_H
#define MUD_FALLOUT2_SESSION_H

#include "unit.h"
#include <string>
#include <mutex>
#include <websocketpp/config/asio.hpp>
#include <websocketpp/server.hpp>

class session {
public:
    session(websocketpp::server<websocketpp::config::asio> *, void *, const std::string &);

    ~session();

    [[maybe_unused]] nlohmann::ordered_json read();

    [[maybe_unused]] nlohmann::ordered_json read(int);

    [[maybe_unused]] void push_message(const std::string &);

    [[maybe_unused]] bool clear();

    [[maybe_unused]] bool notify(const nlohmann::ordered_json &);

    [[maybe_unused]] bool notify(const std::string &, int);

    [[maybe_unused]] bool notify(const std::string &);

    [[maybe_unused]] bool notify(const std::vector<std::string> &);

    [[maybe_unused]] bool notify(const char *);

    [[maybe_unused]] bool notify_stage();

    [[maybe_unused]] bool notify_options();

    [[maybe_unused]] bool execute_option(const std::string &event_index);

    [[maybe_unused]] bool move(const std::string &);

    [[maybe_unused]] bool rest(const std::string &);

    [[maybe_unused]] bool encounter(int);

    [[maybe_unused]] bool notify_messages();

    [[maybe_unused]] std::vector<unit *> random_monsters() const;

    [[maybe_unused]] bool fight(std::vector<unit *> &, std::vector<unit *> &);

    [[maybe_unused]] bool fight(std::vector<unit *> &);

    [[maybe_unused]] unit *get_player() { return this->player; }

    [[maybe_unused]] static unit *login(const std::string &, const std::string &, bool);

protected:
    void main_loop();

    websocketpp::server<websocketpp::config::asio> *websocketpp;
    void *connection_hdl;
    std::string language;
    unit *player;
    std::mutex messages_mutex;
    std::mutex release_mutex;
    std::vector<std::string> messages;
    pthread_t main_thread;
    bool is_shutdown;

    [[maybe_unused]] void release();
};


#endif //MUD_FALLOUT2_SESSION_H
