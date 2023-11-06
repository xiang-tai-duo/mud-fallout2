#pragma once

#include "websocket.h"
#include <websocketpp/common/connection_hdl.hpp>
#include <string>
#include <map>

#define SHARED_PTR(x) ((x).lock().get())

class session;

struct SESSION_INFO {
    class session *engine{};

    websocketpp::connection_hdl connection;

    SESSION_INFO();

    ~SESSION_INFO();
};

void close_session(void *, const std::string &);

void close_session(void *);

void get_sessions(const std::function<void(std::map<void *, struct SESSION_INFO *> *)> &);

void listen();

void send_session_data(void *, const std::string &);