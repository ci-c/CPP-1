#include <gtest/gtest.h>

#include <asio.hpp>

#include <chrono>
#include <csignal>
#include <limits.h>
#include <string>
#include <sys/wait.h>
#include <thread>
#include <unistd.h>

static std::string send_request(unsigned short port, const std::string &req) {
    asio::io_context io;
    asio::ip::tcp::socket s(io);
    s.connect(asio::ip::tcp::endpoint(asio::ip::address::from_string("127.0.0.1"), port));
    asio::write(s, asio::buffer(req + "\n"));
    asio::streambuf buf;
    asio::read_until(s, buf, '\n');
    std::istream is(&buf);
    std::string resp;
    std::getline(is, resp);
    return resp;
}

static std::string daemon_path() {
    char self[PATH_MAX];
    const ssize_t n = readlink("/proc/self/exe", self, sizeof(self) - 1);
    EXPECT_GT(n, 0);
    self[n] = '\0';
    std::string dir(self);
    dir = dir.substr(0, dir.find_last_of('/'));
    return dir + "/iusearchbtw";
}

TEST(TcpIntegrationTest, EndToEnd) {
    // порт должен быть свободен до старта демона
    EXPECT_ANY_THROW(
        send_request(50321, R"({"operation":"+","first_number":1,"second_number":1})"));

    const pid_t pid = fork();
    ASSERT_NE(pid, -1);
    if (pid == 0) {
        const std::string bin = daemon_path();
        execl(bin.c_str(), "iusearchbtw", "--daemon", nullptr);
        _exit(127);
    }

    // busy-цикл до 5 с: ждём пока сервер начнёт слушать
    bool up = false;
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    while (std::chrono::steady_clock::now() < deadline) {
        try {
            asio::io_context io;
            asio::ip::tcp::socket s(io);
            s.connect(asio::ip::tcp::endpoint(asio::ip::address::from_string("127.0.0.1"), 50321));
            up = true;
            break;
        } catch (...) {
            std::this_thread::yield();
        }
    }
    ASSERT_TRUE(up) << "daemon did not start listening on 50321";

    EXPECT_EQ(send_request(50321, R"({"operation":"+","first_number":2,"second_number":3})"), "5");
    EXPECT_EQ(send_request(50321, R"({"operation":"!","first_number":5})"), "120");
    EXPECT_EQ(send_request(50321, R"({"operation":"^","first_number":2,"second_number":3})"), "8");

    const std::string bad = send_request(50321, "not json");
    EXPECT_NE(bad.find("error"), std::string::npos) << "bad response: " << bad;

    kill(pid, SIGTERM);
    int st = 0;
    bool reaped = false;
    const auto wait_deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    while (std::chrono::steady_clock::now() < wait_deadline) {
        const pid_t r = waitpid(pid, &st, WNOHANG);
        if (r == pid) {
            reaped = true;
            break;
        }
        std::this_thread::yield();
    }
    ASSERT_TRUE(reaped) << "daemon did not exit after SIGTERM";
    EXPECT_TRUE(WIFEXITED(st) && WEXITSTATUS(st) == 0);
}
