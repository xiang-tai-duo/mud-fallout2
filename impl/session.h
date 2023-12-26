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

    [[maybe_unused]] nlohmann::ordered_json read_client_json(int);

    [[maybe_unused]] std::string read_client_option(int);

    [[maybe_unused]] void push_message(const std::string &);

    [[maybe_unused]] bool reset_messages();

    [[maybe_unused]] bool wss(const nlohmann::ordered_json &);

    [[maybe_unused]] bool wss(const std::string &, int);

    [[maybe_unused]] bool wss(const std::string &);

    [[maybe_unused]] bool wss(const std::vector<std::string> &);

    [[maybe_unused]] bool send_inline_text_to_client(const std::string &);

    [[maybe_unused]] bool send_stage_info_to_client();

    [[maybe_unused]] bool send_options_to_client();

    [[maybe_unused]] bool execute_option(const std::string &event_index);

    [[maybe_unused]] bool rest(const std::string &);

    [[maybe_unused]] bool confirm_message(const std::string &);

    [[maybe_unused]] bool encounter(int);

    [[maybe_unused]] bool wss();

    [[maybe_unused]] std::vector<unit *> random_monsters() const;

    [[maybe_unused]] bool fight(std::vector<unit *> &, std::vector<unit *> &);

    [[maybe_unused]] bool fight(std::vector<unit *> &);

    [[maybe_unused]] bool login(const std::string &, const std::string &, bool);

    [[maybe_unused]] unit *get_player() { return this->player; }

protected:
    void main_loop();

    websocketpp::server<websocketpp::config::asio> *websocketpp;
    void *connection_hdl;
    std::string language;
    unit *player;
    std::mutex messages_mutex;
    std::mutex release_mutex;
    std::vector<std::string> messages;
    std::thread *main_thread;
    bool is_shutdown;

    [[maybe_unused]] void release();
};


#endif //MUD_FALLOUT2_SESSION_H
