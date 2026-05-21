#include "db.hpp"
#include "math_ops.hpp"
#include <memory>
#include <nlohmann/json.hpp>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>
#include <stdexcept>
#include <string>

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
        spdlog::set_level(spdlog::level::err);
    }
};

struct Task {
    int first_number = 0;
    int second_number = 0;
    char operation = 0;
    int result = 0;
    bool is_cached = false;
};

class TaskWorker {
public:
    TaskWorker(Task &task_);
    virtual ~TaskWorker() = default;
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

class CacheWorker : public TaskWorker {
  public:
    CacheWorker(Task &task_, std::shared_ptr<Database> db_);

  protected:
    std::shared_ptr<Database> db;
    std::string build_key() const;
};

class CacheReader : public CacheWorker {
  public:
    CacheReader(Task &task_, std::shared_ptr<Database> db_);
    void exec() override;
};

class CacheWriter : public CacheWorker {
  public:
    CacheWriter(Task &task_, std::shared_ptr<Database> db_);
    void exec() override;
};

class Runner {
  public:
    int run(int argc, char **argv);
};