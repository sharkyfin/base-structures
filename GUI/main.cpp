#include <charconv>
#include <iostream>
#include <mutex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>

#include "httplib.h"
#include "GUI/Json.hpp"
#include "GUI/TreeSession.hpp"

int ReadNumber(std::string_view text) {
    int value = 0;
    const auto result = std::from_chars(text.data(), text.data() + text.length(), value);
    if (text.empty() || result.ec != std::errc() || result.ptr != text.data() + text.length()) {
        throw std::invalid_argument("Ожидается целое число.");
    }
    return value;
}

void SendError(httplib::Response& response, const std::string& message) {
    std::ostringstream output;
    output << "{\"error\":";
    WriteJsonString(output, message);
    output << '}';
    response.status = 400;
    response.set_content(output.str(), "application/json; charset=utf-8");
}

int main(int argc, char** argv) {
    try {
        if (argc > 2) throw std::invalid_argument("Запуск: avl_visualizer [порт]");
        const int port = argc == 2 ? ReadNumber(argv[1]) : 8080;
        if (port < 1 || port > 65535) throw std::invalid_argument("Порт должен быть от 1 до 65535.");

        TreeSession session;
        std::mutex sessionMutex;
        httplib::Server server;
        // Permit a restart after disconnects, keeping one server per port.
        server.set_socket_options([](auto socket) {
            httplib::set_socket_opt(socket, SOL_SOCKET, SO_REUSEADDR, 1);
        });
        server.set_payload_max_length(1024);
        server.set_default_headers({{"Cache-Control", "no-store"}});
        if (!server.set_mount_point("/", AVL_WEB_DIRECTORY)) throw std::runtime_error("Не найдены файлы интерфейса.");
        server.Get("/api/tree", [&](const httplib::Request&, httplib::Response& response) {
            std::lock_guard<std::mutex> lock(sessionMutex);
            response.set_content(session.ToJson(), "application/json; charset=utf-8");
        });
        server.Post("/api/command", [&](const httplib::Request& request, httplib::Response& response) {
            try {
                const std::string action = request.get_param_value("action");
                const int value = request.has_param("value") ? ReadNumber(request.get_param_value("value")) : 0;
                std::lock_guard<std::mutex> lock(sessionMutex);
                session.Execute(action, value);
                response.set_content(session.ToJson(), "application/json; charset=utf-8");
            } catch (const std::exception& exception) {
                SendError(response, exception.what());
            }
        });
        if (!server.bind_to_port("127.0.0.1", port)) throw std::runtime_error("Порт занят. Укажи другой порт при запуске.");
        std::cout << "AVL · Открой http://127.0.0.1:" << port << "\nОстановка: Ctrl+C\n" << std::flush;
        if (!server.listen_after_bind()) throw std::runtime_error("Не удалось запустить HTTP-сервер.");
    } catch (const std::exception& exception) {
        std::cerr << exception.what() << '\n';
        return 1;
    }
}
