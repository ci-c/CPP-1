#include "runner.hpp"

#include "logger.hpp"

#include <chrono>
#include <exception>
#include <iostream>
#include <poll.h>
#include <thread>
#include <unistd.h>

Runner::Runner()
    : db(), task(), parser(task), checker(task), cache_reader(task, db), calc(task),
      cache_writer(task, db), printer(task) {}

int Runner::run(int argc, char **argv) {
    if (argc < 2) {
        Logger::getInstance().error("Usage: iusearchbtw '<json_string>'");
        return 1;
    }
    return process_one(argv[1]);
}

RequestResult Runner::handle_one(const std::string &input) {
    try {
        task = Task{};
        parser.exec(input);
        checker.exec();
        cache_reader.exec();
        if (!task.is_cached) {
            calc.exec();
            cache_writer.exec();
        }
        return {0, std::to_string(task.result)};
    } catch (const std::exception &e) {
        return {1, e.what()};
    }
}

int Runner::process_one(const std::string &input) {
    const auto r = handle_one(input);
    if (r.status != 0) {
        Logger::getInstance().error("error: " + r.payload);
        return r.status;
    }
    std::cout << r.payload << std::endl;
    return 0;
}

int Runner::run_service() {
    Logger::set_info_level();
    SignalWatcher watcher;
    watcher.start();
    Logger::getInstance().info("service ready");
    std::thread worker(&Runner::service_loop, this, std::ref(watcher));
    worker.join();
    watcher.stop();
    Logger::getInstance().info("service stopped");
    return 0;
}


void Runner::service_loop(SignalWatcher &watcher) {
    struct pollfd pfd{STDIN_FILENO, POLLIN, 0};
    std::string line;
    bool stdin_eof = false;
    while (!watcher.shutdown_requested()) {
        const bool have_data =
            !stdin_eof && (std::cin.rdbuf()->in_avail() > 0 || ::poll(&pfd, 1, 100) > 0);
        if (watcher.shutdown_requested())
            break;
        if (have_data) {
            if (!std::getline(std::cin, line)) {
                stdin_eof = true;
                continue;
            }
            if (!line.empty())
                process_one(line);
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
}
