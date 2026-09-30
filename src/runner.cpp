#include "runner.hpp"

#include "logger.hpp"

#include <systemd/sd-daemon.h>

#include <exception>
#include <iostream>
#include <thread>

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
    sd_notify(0, "READY=1");
    Logger::getInstance().info("service ready");
    std::thread worker(&Runner::service_loop, this, std::ref(watcher));
    worker.join();
    watcher.stop();
    sd_notify(0, "STOPPING=1");
    Logger::getInstance().info("service stopped");
    return 0;
}

void Runner::service_loop(SignalWatcher &watcher) {
    TcpServer server(port_, [this](const std::string &req) {
        const auto r = handle_one(req);
        if (r.status == 0)
            return r.payload;
        return std::string("{\"error\":\"") + r.payload + "\"}";
    });
    server.run([&watcher] { return watcher.shutdown_requested(); });
}
