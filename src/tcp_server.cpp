#include "tcp_server.hpp"

#include "logger.hpp"

#include <istream>
#include <ostream>
#include <poll.h>
#include <system_error>

TcpServer::TcpServer(unsigned short port, std::function<std::string(const std::string &)> handler)
    : acceptor_(io_, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port)),
      handler_(std::move(handler)) {}

void TcpServer::run(std::function<bool()> should_stop) {
    while (!should_stop()) {
        struct pollfd pfd{acceptor_.native_handle(), POLLIN, 0};
        if (::poll(&pfd, 1, 100) <= 0) {
            continue; // таймаут — перепроверить should_stop
        }
        asio::ip::tcp::socket socket(io_);
        std::error_code ec;
        acceptor_.accept(socket, ec);
        if (ec) {
            if (should_stop())
                break;
            Logger::getInstance().error("accept: " + ec.message());
            continue;
        }
        try {
            asio::streambuf buf;
            asio::read_until(socket, buf, '\n');
            std::istream is(&buf);
            std::string req;
            std::getline(is, req);
            const std::string resp = handler_(req);
            asio::write(socket, asio::buffer(resp + "\n"));
        } catch (const std::system_error &e) {
            Logger::getInstance().error(std::string("client: ") + e.what());
        }
    }
    acceptor_.close();
}
