#pragma once

#include "math_ops.hpp"
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <string>
#include <memory>
#include <stdexcept>

class Logger {
public:
    static Logger& getInstance() {
        static Logger instance;
        return instance;
    }
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    void info(const std::string& message) { spd_logger->info(message); }
    void error(const std::string& message) { spd_logger->error(message); }

private:
    std::shared_ptr<spdlog::logger> spd_logger;
    Logger() {
        spd_logger = spdlog::stdout_color_mt("console");
        spdlog::set_level(spdlog::level::debug);
    }
};

struct Task {
    int first_number = 0;
    int second_number = 0;
    char operation = 0;
    int result = 0;
};

class TaskWorker {
public:
    TaskWorker(Task &task_);
    virtual ~TaskWorker() = default; // Виртуальный деструктор - правило хорошего тона
    Task &getResult();
    virtual void exec() = 0;

protected:
    Task &task;
};

class Calculator : public TaskWorker { 
public:
    Calculator(Task &task_);
    void exec() override;
};

class Parser : public TaskWorker {
public:
    Parser(Task &task_, const std::string& input);
    void exec() override;
private:
    std::string json_str;
};

class Checker : public TaskWorker {
public:
    Checker(Task &task_);
    void exec() override;
private:
    bool is_binary_operation(char operation) const;
};

class Printer : public TaskWorker {
public:
    Printer(Task &task_);
    void exec() override;
};

class Runner {
public:
    int run(int argc, char **argv);
};