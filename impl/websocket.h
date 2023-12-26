#pragma once

#include "websocket.h"
#include <websocketpp/common/connection_hdl.hpp>
#include <functional>
#include <string>
#include <map>

#define SHARED_PTR(x) ((x).lock().get())

class session;

struct WSS {
    class session *session{};

    websocketpp::connection_hdl connection;

    WSS();

    ~WSS();
};

void close_wss_client(void *hdl, const std::string &reason);

void close_wss_client(void *);

void enum_wss(const std::function<void(std::map<void *, struct WSS *> *)> &fn);

void listen();

void send_wss_data(void *hdl, const std::string &data);