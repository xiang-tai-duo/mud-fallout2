//
// Created by 黄元镭 on 2023/7/20.
//

#ifndef MUD_FALLOUT_UTILS_H
#define MUD_FALLOUT_UTILS_H

#ifdef WINDOWS
#else
#define _LIBCPP_DISABLE_DEPRECATION_WARNINGS
#endif

#include <string>
#include <utility>
#include <vector>
#include <iostream>
#include <string>
#include <cstdint>
#include <sstream>
#include <random>
#include <fstream>
#include <thread>
#include <mutex>
#include <filesystem>
#include "nlohmann/json.hpp"

#ifdef WINDOWS

#include <cstdarg>
#include <memory>
#include <cstring>
#include <locale>
#include <codecvt>
#include <Windows.h>

#else

#include <iconv.h>

#endif

#ifdef ARRAYSIZE
#undef ARRAYSIZE
#endif
#define ARRAYSIZE(arr) (sizeof(arr) / sizeof(arr[0]))

#ifndef MIN
#define MIN(x, y) ((x) > (y) ? (y) : (x))
#endif

#ifdef RELEASE
#define TRACE(x) (x)
#else
#define TRACE utils::console::trace
#endif

#ifdef WINDOWS
#define CP_936                     936
#define UTF8(x)                    utils::encoding::gbk_to_utf8(x)
#define GBK(x)                     utils::encoding::utf8_to_gbk(x)
#else
#define UTF8(x)                    std::string(x)
#define GBK(x)                     std::string(x)
#endif

#define NOW (std::chrono::high_resolution_clock::now())

#define VA_INIT_WIN(var_placeholder, format)                                        \
    std::string s;                                                                  \
    va_list args;                                                                   \
    va_start(args, var_placeholder);                                                \
    auto size = (_vscprintf(format.c_str(), args) + 1) * sizeof(char);              \
    va_end(args);                                                                   \
    auto p = std::shared_ptr<char>(new char[size]);                                 \
    memset(p.get(), 0, size);                                                       \
    va_start(args, var_placeholder);                                                \
    _vsnprintf_s(p.get(), size, size - 1, format.c_str(), args);                    \
    va_end(args);                                                                   \
    s = p.get()

#define VA_INIT_MAC(var_placeholder, format)                                        \
    std::string s;                                                                  \
    va_list args;                                                                   \
    va_start(args, var_placeholder);                                                \
    auto size = (vsnprintf(nullptr, 0, format.c_str(), args) + 1) * sizeof(char);   \
    va_end(args);                                                                   \
    auto p = std::shared_ptr<char>(new char[size]);                                 \
    memset(p.get(), 0, size);                                                       \
    va_start(args, var_placeholder);                                                \
    vsnprintf(p.get(), size, format.c_str(), args);                                 \
    va_end(args);                                                                   \
    s = p.get()

#ifdef WINDOWS
#define VA_INIT VA_INIT_WIN
#else
#define VA_INIT VA_INIT_MAC
#endif

namespace utils {
    namespace datetime {
        inline std::string now(const std::string &);

        inline std::string now();

        inline long long duration(std::chrono::time_point<std::chrono::steady_clock> &);
    }

    namespace console {
        inline void trace(std::string f, ...);

        inline void critical(std::string f, ...);
    }

    namespace strings {
        inline std::string format(std::string f, ...);

        inline std::string replace(const std::string &, const std::string &, const std::string &);
    }

    namespace encoding {
#ifdef WINDOWS
#define CP_936 936

        inline std::string utf8_to_codepage(const std::string &, int);

        inline std::string utf8_to_mbcs(const std::string &);

        inline std::string utf8_to_gbk(const std::string &);

        inline std::string gbk_to_utf8(const std::string &);

#endif
    }
}
namespace utils {
    namespace console {
        static std::mutex trace_mutex;

        inline std::string generate_colors_string() {
            std::string s;
            s += strings::format("\x1b[30m\\x1b[30m\x1b[0m\n");
            s += strings::format("\x1b[31m\\x1b[31m\x1b[0m\n");
            s += strings::format("\x1b[32m\\x1b[32m\x1b[0m\n");
            s += strings::format("\x1b[33m\\x1b[33m\x1b[0m\n");
            s += strings::format("\x1b[34m\\x1b[34m\x1b[0m\n");
            s += strings::format("\x1b[35m\\x1b[35m\x1b[0m\n");
            s += strings::format("\x1b[36m\\x1b[36m\x1b[0m\n");
            s += strings::format("\x1b[37m\\x1b[37m\x1b[0m\n");
            s += strings::format("\x1b[40;37m\\x1b[40;37m\x1b[0m\n");
            s += strings::format("\x1b[41;37m\\x1b[41;37m\x1b[0m\n");
            s += strings::format("\x1b[42;37m\\x1b[42;37m\x1b[0m\n");
            s += strings::format("\x1b[43;37m\\x1b[43;37m\x1b[0m\n");
            s += strings::format("\x1b[44;37m\\x1b[44;37m\x1b[0m\n");
            s += strings::format("\x1b[45;37m\\x1b[45;37m\x1b[0m\n");
            s += strings::format("\x1b[46;37m\\x1b[46;37m\x1b[0m\n");
            s += strings::format("\x1b[47;37m\\x1b[47;37m\x1b[0m\n");
            s += strings::format("\x1b[90;30m\\x1b[90;30m\x1b[0m\n");
            s += strings::format("\x1b[91;31m\\x1b[91;31m\x1b[0m\n");
            s += strings::format("\x1b[92;32m\\x1b[92;32m\x1b[0m\n");
            s += strings::format("\x1b[93;33m\\x1b[93;33m\x1b[0m\n");
            s += strings::format("\x1b[94;34m\\x1b[94;34m\x1b[0m\n");
            s += strings::format("\x1b[95;35m\\x1b[95;35m\x1b[0m\n");
            s += strings::format("\x1b[96;36m\\x1b[96;36m\x1b[0m\n");
            s += strings::format("\x1b[97;37m\\x1b[97;37m\x1b[0m\n");
            s += strings::format("\x1b[100;90;40;37m\\x1b[100;90;40;37m\x1b[0m\n");
            s += strings::format("\x1b[101;91;41;37m\\x1b[101;91;41;37m\x1b[0m\n");
            s += strings::format("\x1b[102;92;42;37m\\x1b[102;92;42;37m\x1b[0m\n");
            s += strings::format("\x1b[103;93;43;37m\\x1b[103;93;43;37m\x1b[0m\n");
            s += strings::format("\x1b[104;94;44;37m\\x1b[104;94;44;37m\x1b[0m\n");
            s += strings::format("\x1b[105;95;45;37m\\x1b[105;95;45;37m\x1b[0m\n");
            s += strings::format("\x1b[106;96;46;37m\\x1b[106;96;46;37m\x1b[0m\n");
            s += strings::format("\x1b[107;97;47;37m\\x1b[107;97;47;37m\x1b[0m\n");
            return s;
        }

        inline void trace(const std::string f, ...) {
            trace_mutex.lock();
            VA_INIT(f, f);
#ifdef WINDOWS
            printf("[%s] %s\n", datetime::now().c_str(), UTF8(s).c_str());
#else
            printf("[%s] %s\n", datetime::now().c_str(), s.c_str());
#endif
            fflush(stdout);
            trace_mutex.unlock();
        }

        inline void critical(const std::string f, ...) {
            VA_INIT(f, f);
#ifdef WINDOWS
            printf("[%s] \x1b[31m%s\x1b[0m\n", datetime::now().c_str(), UTF8(s).c_str());
#else
            printf("[%s] \x1b[31m%s\x1b[0m\n", datetime::now().c_str(), s.c_str());
#endif
            fflush(stdout);
        }

        inline void _throw(const std::string f, ...) {
            VA_INIT(f, f);
            printf("[%s] \x1b[31m%s\x1b[0m\n", datetime::now().c_str(), s.c_str());
            fflush(stdout);
            throw;
        }

        inline std::string yellow(const std::string &s) {
            return strings::format("\x1b[33m%s\x1b[0m", s.c_str());
        }

        inline std::string green(const std::string &s) {
            return strings::format("\x1b[32m%s\x1b[0m", s.c_str());
        }

        inline std::string red(const std::string &s) {
            return strings::format("\x1b[31m%s\x1b[0m", s.c_str());
        }

        inline std::string clear_terminal_control_symbol(const std::string &s) {
            auto result = std::string(s);
            for (auto i = 0; i <= 100; i++) {
                result = strings::replace(result, strings::format("\x1b[%dm", i), "");
            }
            return result;
        }
    }

    namespace strings {
        inline bool starts_with(const std::string &source, const std::string &find) {
            if (&source == &find) return true;
            const typename std::string::size_type big_size = source.size();
            const typename std::string::size_type small_size = find.size();
            const bool valid_ = (big_size >= small_size);
            const bool starts_with_ = (source.compare(0, small_size, find) == 0);
            return valid_ && starts_with_;
        }

        inline bool ends_with(const std::string &big, const std::string &small) {
            if (&big == &small) return true;
            const typename std::string::size_type big_size = big.size();
            const typename std::string::size_type small_size = small.size();
            const bool valid_ = (big_size >= small_size);
            const bool ends_with_ = (big.compare(big_size - small_size, small_size, small) == 0);
            return valid_ && ends_with_;
        }

        inline std::string format(std::string f, ...) {
            VA_INIT(f, f);
            return s;
        }

        inline int atoi(const std::string a) {
            auto ptr = (char *) nullptr;
            return (int) strtol(a.c_str(), &ptr, 10);
        }

        inline std::string itoa(int i) {
            return format("%d", i);
        }

        inline std::string replace(std::string s, const std::string &target, const std::string &replacement, bool replace_first) {
            if (s.empty() || target.empty()) {
                return s;
            }
            using S = std::string;
            using C = std::string::value_type;
            using N = std::string::size_type;
            struct {
                static auto len(const S &s) { return s.size(); }

                static auto len(const C *p) { return std::char_traits<C>::length(p); }

                static auto len(const C c) { return sizeof(c); }

                static auto sub(S *s, const S &t, N pos, N len) { s->replace(pos, len, t); }

                static auto sub(S *s, const C *t, N pos, N len) { s->replace(pos, len, t); }

                static auto sub(S *s, const C t, N pos, N len) { s->replace(pos, len, 1, t); }

                static auto ins(S *s, const S &t, N pos) { s->insert(pos, t); }

                static auto ins(S *s, const C *t, N pos) { s->insert(pos, t); }

                static auto ins(S *s, const C t, N pos) { s->insert(pos, 1, t); }
            } util;

            N target_length = util.len(target);
            N replacement_length = util.len(replacement);
            N pos = 0;
            while ((pos = s.find(target, pos)) != std::string::npos) {
                util.sub(&s, replacement, pos, target_length);
                if (replace_first) return s;
                pos += replacement_length;
            }
            return s;
        }

        inline std::string replace(const std::string &s, const std::string &target, const std::string &replacement) {
            return replace(s, target, replacement, false);
        }

        inline std::string random(int size = 8) {
            static const char alphanum[] =
                    "0123456789"
                    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                    "abcdefghijklmnopqrstuvwxyz";
            std::string s;
            s.reserve(size);
            for (int i = 0; i < size; ++i) {
                s += alphanum[rand() % (sizeof(alphanum) - 1)];
            }
            return s;
        }

        inline std::vector<std::string> split(const std::string &input, const char delimiter) {
            std::vector<std::string> result;
            std::istringstream iss(input);
            std::string token;
            while (std::getline(iss, token, delimiter)) {
                result.push_back(token);
            }
            return result;
        }

        inline std::vector<std::string> split(const std::string &input, const std::string &delimiter) {
            std::vector<std::string> result;
            std::string s = input;
            size_t pos;
            while ((pos = s.find(delimiter)) != std::string::npos) {
                result.push_back(s.substr(0, pos));
                s.erase(0, pos + delimiter.length());
            }
            if (!s.empty()) {
                result.push_back(s);
            }
            return result; // 返回结果
        }

        inline std::string remove_escape_characters(const std::string& s) {
            return utils::strings::replace(s, "%", "%%");
        }
    }

    namespace encoding {

#ifdef WINDOWS

        inline std::string utf8_to_codepage(const std::string &utf8_str, int code_page) {
            auto s = std::string();
            int size = MultiByteToWideChar(CP_UTF8, 0, utf8_str.c_str(), -1, nullptr, 0);
            if (size > 0) {
                std::wstring wide_str(size, L'\0');
                if (MultiByteToWideChar(CP_UTF8, 0, utf8_str.c_str(), -1, &wide_str[0], size) >= 0) {
                    size = WideCharToMultiByte(code_page, 0, wide_str.c_str(), -1, nullptr, 0, nullptr, nullptr);
                    s.resize(size);
                    WideCharToMultiByte(code_page, 0, wide_str.c_str(), -1, &s[0], size, nullptr, nullptr);
                    if (!s.empty() && s[s.size() - 1] == '\0') {
                        s.resize(s.size() - 1);
                    }
                }
            }
            return s;
        }

        inline std::string utf8_to_mbcs(const std::string &utf8_str) {
            return utf8_to_codepage(utf8_str, CP_ACP);
        }

        inline std::string utf8_to_gbk(const std::string &utf8_str) {
            return utf8_to_codepage(utf8_str, CP_936);
        }

        inline std::string gbk_to_utf8(const std::string &gbk) {
            auto wideCharLength = MultiByteToWideChar(CP_936, 0, gbk.c_str(), (int) gbk.size(), nullptr, 0);
            auto wideChar = new wchar_t[wideCharLength];
            MultiByteToWideChar(CP_936, 0, gbk.c_str(), (int) gbk.size(), wideChar, wideCharLength);
            auto utf8Length = WideCharToMultiByte(CP_UTF8, 0, wideChar, wideCharLength, nullptr, 0, nullptr, nullptr);
            auto utf8 = new char[utf8Length];
            WideCharToMultiByte(CP_UTF8, 0, wideChar, wideCharLength, utf8, utf8Length, nullptr, nullptr);
            std::string result(utf8, utf8Length);
            delete[] wideChar;
            delete[] utf8;
            return result;
        }

#endif
    }

    namespace json {
        inline nlohmann::ordered_json get_object(const nlohmann::ordered_json &json, const std::string &key) {
            auto value = nlohmann::ordered_json();
            if (json.is_object() && json.contains(key)) {
                value = json[key];
            }
            return value;
        }

        inline std::string get_string(const nlohmann::ordered_json &json, const std::string &key) {
            auto value = std::string();
            if (json.contains(key) && json[key].is_string()) {
                value = GBK(json[key].get<std::string>());
            }
            return value;
        }

        inline int get_integer(const nlohmann::ordered_json &json, const std::string &key) {
            auto value = 0;
            if (json.is_object() && json.contains(key) && json[key].is_number_integer()) {
                value = json[key].get<int>();
            }
            return value;
        }

        inline int get_boolean(const nlohmann::ordered_json &json, const std::string &key) {
            auto value = 0;
            if (json.is_object() && json.contains(key) && json[key].is_boolean()) {
                value = json[key].get<bool>();
            }
            return value;
        }

        inline nlohmann::ordered_json get_array(const nlohmann::ordered_json &json, const std::string &key) {
            auto value = nlohmann::ordered_json::array();
            if (json.is_object() && json.contains(key) && json[key].is_array()) {
                value = json[key];
            }
            return value;
        }

        inline std::vector<std::string> get_strings(const nlohmann::ordered_json &json, const std::string &key) {
            auto array = std::vector<std::string>();
            if (json.is_object() && json.contains(key)) {
                if (json[key].is_array()) {
                    for (const auto &it: json[key]) {
                        if (it.is_string()) {
                            array.emplace_back(GBK(it.get<std::string>()));
                        }
                    }
                } else if (json[key].is_string()) {
                    array.emplace_back(GBK(json[key].get<std::string>()));
                }
            }
            return array;
        }
    }

    namespace math {
        inline int random(int min, int max) {
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<int> dis(min, max);
            return dis(gen);
        }

        inline int random() {
            return random(1, 10);
        }

#ifdef max
#undef max
#endif

        inline int max(int a, int b) {
            return a > b ? a : b;
        }
    }

    namespace datetime {
        inline std::string now(const std::string &format) {
            std::time_t now_c = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
            std::tm *now_tm = std::localtime(&now_c);
            std::stringstream ss;
            ss << std::put_time(now_tm, format.c_str());
            return ss.str();
        }

        inline std::string now() {
            return now("%Y-%m-%d %H:%M:%S");
        }

        inline long long duration(std::chrono::time_point<std::chrono::steady_clock> &start_time) {
            return std::chrono::duration_cast<std::chrono::milliseconds>(NOW -start_time).count();
        }
    }

    namespace directory {
        inline bool is_exists(const std::string &path) {
            auto p = std::filesystem::path(path);
            return std::filesystem::exists(p) && std::filesystem::is_directory(p);
        }

        inline std::vector<std::string> get_files(const std::string &path) {
            auto files = std::vector<std::string>();
            if (is_exists(path)) {
                for (const auto &entry: std::filesystem::directory_iterator(std::filesystem::path(path))) {
                    if (entry.is_regular_file()) {
                        files.emplace_back(entry.path().string());
                    }
                }
            }
            return files;
        }
    }

    namespace file {
        inline bool is_exists(const std::string &path) {
            return std::filesystem::exists(std::filesystem::path(path));
        }

        inline std::string load(const std::string &path) {
            std::ostringstream ss;
            if (is_exists(path)) {
                std::ifstream file(path);
                if (file.is_open()) {
                    ss << file.rdbuf();
                    file.close();
                }
            }
            return ss.str();
        }

        inline void save(const std::string &file_path, const std::string &file_content) {
            std::ofstream file(file_path);
            if (file.is_open()) {
                file << file_content;
                file.close();
            }
        }
    }

    namespace threading {
        inline void sleep(int milliseconds) {
            std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
        }

        inline void sleep_seconds(int seconds) {
            std::this_thread::sleep_for(std::chrono::seconds(seconds));
        }
    }

    static class _init_ {
    public:
        _init_() {
            srand(time(nullptr));
        }
    } _init_;
}
#endif //MUD_FALLOUT_UTILS_H
