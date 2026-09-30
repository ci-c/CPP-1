#pragma once

#include <asio.hpp>
#include <functional>
#include <string>

class TcpServer {
  public:
    TcpServer(unsigned short port, std::function<std::string(const std::string &)> handler);
    void run(std::function<bool()> should_stop); // accept/read/write цикл
  private:
    asio::io_context io_;
    asio::ip::tcp::acceptor acceptor_;
    std::function<std::string(const std::string &)> handler_;
};
