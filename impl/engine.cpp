//
// Created by 黄元镭 on 2023/8/3.
//

#include "engine.h"

#define KEYWORD_JSON_FILE_NAME "Config/keywords.json"

namespace mud_fallout2::engine {
    void launch() {
        load_keywords();
        std::vector<std::thread *> client_threads;
        auto server_socket = socket(AF_INET, SOCK_STREAM, 0);
        if (server_socket != SOCKET_ERROR) {
            int enable = 1;
            if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char *>(&enable), sizeof(enable)) != SOCKET_ERROR) {
                struct sockaddr_in server_address{};
                server_address.sin_family = AF_INET;
                server_address.sin_addr.s_addr = INADDR_ANY;
                server_address.sin_port = htons(LISTEN_PORT);
                while (bind(server_socket, (struct sockaddr *) &server_address, sizeof(server_address)) == SOCKET_ERROR) {
                    utils::console::trace("Bind failed, wait for 1 second");
                    utils::threading::sleep_seconds(1);
                }
                if (listen(server_socket, 1) != SOCKET_ERROR) {
                    TRACE("mud_fallout2 telnet server listen on ::%d", LISTEN_PORT);
                    TRACE("JSON format: https://www.sojson.com");
                    struct sockaddr_in client_address{};
                    socklen_t client_address_size = sizeof(client_address);
                    while (!is_shutdown) {
                        auto client_socket = accept(server_socket, (struct sockaddr *) &client_address, &client_address_size);
                        if (client_socket != SOCKET_ERROR) {
                            client_threads.emplace_back(invoke(client_socket));
                        }
                    }
                }
            }
#ifdef WINDOWS
            closesocket(server_socket);
#else
            close(server_socket);
#endif
        }
        utils::console::trace(utils::strings::format("Unreachable"));
    }

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

    inline std::thread *invoke(int client_socket) {
        return new std::thread([](int client_socket) -> void {
            session session(client_socket);
        }, client_socket);
    }
}
