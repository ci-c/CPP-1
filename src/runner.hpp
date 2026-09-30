#pragma once

#include <string>

#include "cache_reader.hpp"
#include "cache_writer.hpp"
#include "calculator.hpp"
#include "checker.hpp"
#include "db.hpp"
#include "parser.hpp"
#include "printer.hpp"
#include "signal_watcher.hpp"
#include "task.hpp"
#include "tcp_server.hpp"

struct RequestResult {
    int status = 1;      // 0 = OK, 1 = ошибка
    std::string payload; // OK: число; ошибка: e.what()
};

class Runner {
  public:
    Runner();
    int run(int argc, char **argv);
    int run_service();

  private:
    int process_one(const std::string &input);
    RequestResult handle_one(const std::string &input);
    void service_loop(SignalWatcher &watcher);

    Database db;
    Task task;
    Parser parser;
    Checker checker;
    CacheReader cache_reader;
    Calculator calc;
    CacheWriter cache_writer;
    Printer printer;
    unsigned short port_ = 50321;
};
