#pragma clang diagnostic push
#pragma ide diagnostic ignored "modernize-avoid-bind"

#include "websocket.h"
#include "session.h"
#include <iostream>

using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;
using websocketpp::lib::bind;

websocketpp::server<websocketpp::config::asio> websocket;
std::map<void *, struct SESSION_INFO *> sessions;
std::mutex sessions_mutex;

SESSION_INFO::SESSION_INFO() {
    this->engine = nullptr;
}

SESSION_INFO::~SESSION_INFO() {
    if (this->engine) {
        delete this->engine;
        this->engine = nullptr;
    }
}

// Define a callback to handle incoming messages
void on_message(websocketpp::server<websocketpp::config::asio> *s,
                const websocketpp::connection_hdl &hdl,
                const websocketpp::server<websocketpp::config::asio>::message_ptr &msg) {
    auto ptr = SHARED_PTR(hdl);
    sessions_mutex.lock();
    if (sessions.find(ptr) != sessions.end()) {
        auto session = sessions[ptr];
        if (session && session->engine) {
            session->engine->push_message(msg->get_payload());
        }
    }
    sessions_mutex.unlock();
}

void on_open(websocketpp::server<websocketpp::config::asio> *s, websocketpp::connection_hdl hdl) {
    auto ptr = SHARED_PTR(hdl);
    sessions_mutex.lock();
    auto session_info = new SESSION_INFO();
    session_info->engine = new class session(s, ptr, "zh");
    session_info->connection = hdl;
    sessions[ptr] = session_info;
    sessions_mutex.unlock();
}

void close_session(void *hdl, const std::string &reason) {
    sessions_mutex.lock();
    if (sessions.find(hdl) != sessions.end()) {
        delete sessions[hdl];
        sessions.erase(hdl);
    }
    sessions_mutex.unlock();
}

void close_session(void *hdl) {
    close_session(hdl, std::string());
}

void get_sessions(const std::function<void(std::map<void *, struct SESSION_INFO *> *)> &fn) {
    sessions_mutex.lock();
    fn(&sessions);
    sessions_mutex.unlock();
}

void on_close(websocketpp::server<websocketpp::config::asio> *s, const websocketpp::connection_hdl &hdl) {
    auto ptr = SHARED_PTR(hdl);
    sessions_mutex.lock();
    if (sessions.find(ptr) != sessions.end()) {
        delete sessions[ptr];
        sessions.erase(ptr);
    }
    sessions_mutex.unlock();
}

void listen() {
    try {
        // Set logging settings
        websocket.clear_access_channels(websocketpp::log::alevel::frame_header | websocketpp::log::alevel::frame_payload);
        websocket.clear_access_channels(websocketpp::log::alevel::all);

        // Initialize Asio
        websocket.init_asio();

        // Register our message handler
        websocket.set_message_handler(bind(&on_message, &websocket, ::_1, ::_2));
        websocket.set_open_handler(bind(&on_open, &websocket, ::_1));
        websocket.set_close_handler(bind(&on_close, &websocket, ::_1));

        auto listened = false;
        while (!listened) {
            try {
                // Listen on port 9002
                websocket.listen(9002);
                listened = true;
            } catch (...) {
                utils::threading::sleep(1000);
            }
        }

        utils::console::trace("websocket is ready.");

        // Start the server accept loop
        websocket.start_accept();

        // Start the ASIO io_service run loop
        websocket.run();
    } catch (websocketpp::exception const &e) {
        std::cout << e.what() << std::endl;
    } catch (...) {
        std::cout << "other exception" << std::endl;
    }
}

void send_session_data(void *hdl, const std::string &data) {
    sessions_mutex.lock();
    if (sessions.find(hdl) != sessions.end()) {
        websocket.send(sessions[hdl]->connection, data, websocketpp::frame::opcode::value::TEXT);
    }
    sessions_mutex.unlock();

}

#pragma clang diagnostic pop
