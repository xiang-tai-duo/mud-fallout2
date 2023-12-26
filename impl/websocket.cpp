#ifdef MAC
#pragma clang diagnostic push
#pragma ide diagnostic ignored "modernize-avoid-bind"
#endif

#include "macros.h"
#include "websocket.h"
#include "session.h"
#include <iostream>

using websocketpp::lib::placeholders::_1; // NOLINT(*-reserved-identifier)
using websocketpp::lib::placeholders::_2; // NOLINT(*-reserved-identifier)
using websocketpp::lib::bind;

websocketpp::server<websocketpp::config::asio> websocket;
std::map<void *, struct WSS *> exists_wss;
std::mutex wss_mutex;

WSS::WSS() {
    this->session = nullptr;
}

WSS::~WSS() {
    if (this->session) {
        delete this->session;
        this->session = nullptr;
    }
}

WSS *find_exists_wss(void *connection) {
    struct WSS *client = nullptr;
    wss_mutex.lock();
    if (exists_wss.find(connection) != exists_wss.end()) {
        client = exists_wss[connection];
    }
    wss_mutex.unlock();
    return client;
}

void wss_connected(websocketpp::server<websocketpp::config::asio> *s, websocketpp::connection_hdl hdl) {
    auto ptr = SHARED_PTR(hdl);
    auto wss = new WSS();
    wss->session = new class session(s, ptr, "zh");
    wss->connection = hdl;
    wss_mutex.lock();
    utils::console::trace("A new wss is coming, hdl: 0x%x", wss->connection.lock().get());
    exists_wss[ptr] = wss;
    wss_mutex.unlock();
}

// Define a callback to handle incoming messages
void wss_message_arrived(websocketpp::server<websocketpp::config::asio> *s,
                         const websocketpp::connection_hdl &hdl,
                         const websocketpp::server<websocketpp::config::asio>::message_ptr &msg) {
    auto wss = find_exists_wss(SHARED_PTR(hdl));
    if (wss && wss->session) {
        wss->session->push_message(msg->get_payload());
    }
}

void wss_closed(websocketpp::server<websocketpp::config::asio> *s, const websocketpp::connection_hdl &hdl) {
    auto wss = find_exists_wss(SHARED_PTR(hdl));
    if (wss) {
        wss_mutex.lock();
        exists_wss.erase(SHARED_PTR(hdl));
        wss_mutex.unlock();
        delete wss;
    }
}

void close_wss_client(void *hdl) {
    auto wss = find_exists_wss(hdl);
    if (wss) {
        wss_mutex.lock();
        exists_wss.erase(hdl);
        if (wss->session) {
            delete wss->session;
            wss->session = nullptr;
        }
        websocket.close(wss->connection, 0, std::string());
        delete wss;
        wss_mutex.unlock();
    }
}

void enum_wss(const std::function<void(std::map<void *, struct WSS *> *)> &fn) {
    wss_mutex.lock();
    fn(&exists_wss);
    wss_mutex.unlock();
}

void listen() {
    try {
        // Set logging settings
        websocket.clear_access_channels(websocketpp::log::alevel::frame_header | websocketpp::log::alevel::frame_payload);
        websocket.clear_access_channels(websocketpp::log::alevel::all);

        // Initialize Asio
        websocket.init_asio();

        // Set reuse socket
        websocket.set_reuse_addr(true);

        // Register our message handler
        websocket.set_message_handler(bind(&wss_message_arrived, &websocket, ::_1, ::_2));
        websocket.set_open_handler(bind(&wss_connected, &websocket, ::_1));
        websocket.set_close_handler(bind(&wss_closed, &websocket, ::_1));

        auto listened = false;
        while (!listened) {
            try {
                // Listen on port 9002
                websocket.listen(WEBSOCKET_PORT);
                listened = true;
            } catch (...) {
                utils::threading::sleep(1000);
            }
        }

        utils::console::trace("websocket is listening on ::%d", WEBSOCKET_PORT);

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

void send_wss_data(void *hdl, const std::string &data) {
    auto wss = find_exists_wss(hdl);
    if (wss) {
        websocket.send(wss->connection, data, websocketpp::frame::opcode::value::TEXT);
    }
}

#ifdef MAC
#pragma clang diagnostic pop
#endif
