#include "app.hpp"
#include <stdio.h>

// --- TaskWorker ---
TaskWorker::TaskWorker(Task &task_) : task{task_} {}
Task &TaskWorker::getResult() { return task; }

// --- Calculator ---
Calculator::Calculator(Task &task_) : TaskWorker(task_) {}

void Calculator::exec() {
    int math_result = 0;
    switch (task.operation) {
    case '+':
        mathlib::add(task.first_number, task.second_number, &math_result);
        break;
    case '-':
        mathlib::sub(task.first_number, task.second_number, &math_result);
        break;
    case '*':
        mathlib::mul(task.first_number, task.second_number, &math_result);
        break;
    case '/':
        mathlib::divide(task.first_number, task.second_number, &math_result);
        break;
    case '^':
        mathlib::power(task.first_number, task.second_number, &math_result);
        break;
    case '!':
        mathlib::factorial(task.first_number, &math_result);
        break;
    default:
        throw std::runtime_error("Invalid operation");
    }
    task.result = math_result;
}

// --- Parser ---
Parser::Parser(Task &task_) : TaskWorker(task_) {}

void Parser::exec(const std::string &input) {
    using json = nlohmann::json;
    json json_input = json::parse(input);

    std::string op_str = json_input["operation"].get<std::string>();
    task.operation = op_str[0];

    task.first_number = json_input["first_number"].get<int>();
    if (json_input.contains("second_number")) {
        task.second_number = json_input["second_number"].get<int>();
    }
}

// --- Checker ---
Checker::Checker(Task &task_) : TaskWorker(task_) {}

void Checker::exec() {
    if (!is_binary_operation(task.operation) && task.operation != '!') {
        throw std::invalid_argument("Unknown operation");
    }
    if (is_binary_operation(task.operation)) {
        if (task.operation == '/' && task.second_number == 0) {
            throw std::invalid_argument("Division by zero");
        }
        if (task.operation == '^' && task.second_number < 0) {
            throw std::invalid_argument("Negative power is not supported");
        }
        return;
    }
    if (task.operation == '!') {
        if (task.first_number < 0) {
            throw std::invalid_argument("Factorial of negative number");
        }
        return;
    }
}

bool Checker::is_binary_operation(char operation) const {
    switch (operation) {
    case '+':
    case '-':
    case '*':
    case '/':
    case '^':
        return true;
    default:
        return false;
    }
}

// --- Printer ---
Printer::Printer(Task &task_) : TaskWorker(task_) {}

void Printer::exec() { printf("%d\n", task.result); }

// --- CacheWorker ---
CacheWorker::CacheWorker(Task &task_, Database &db_) : TaskWorker(task_), db(db_) {}

std::string CacheWorker::build_key() const {
    int a = task.first_number;
    int b = task.second_number;
    switch (task.operation) {
    case '+':
        if (a < b)
            std::swap(a, b);
        return std::to_string(a) + "+" + std::to_string(b);
    case '-':
        return std::to_string(a) + "-" + std::to_string(b);
    case '*':
        if (a < b)
            std::swap(a, b);
        return std::to_string(a) + "*" + std::to_string(b);
    case '/':
        return std::to_string(a) + "/" + std::to_string(b);
    case '^':
        return std::to_string(a) + "^" + std::to_string(b);
    case '!':
        return "!" + std::to_string(a);
    default:
        throw std::runtime_error("Invalid operation");
    }
}

// --- CacheReader ---
CacheReader::CacheReader(Task &task_, Database &db_) : CacheWorker(task_, db_) {}

void CacheReader::exec() {
    const std::string key = build_key();
    auto cached = db.read(key);
    if (cached) {
        task.result = std::stoi(*cached);
        task.is_cached = true;
        Logger::getInstance().info("cache hit: " + key + " = " + *cached);
    } else {
        Logger::getInstance().info("cache miss: " + key);
    }
}

// --- CacheWriter ---
CacheWriter::CacheWriter(Task &task_, Database &db_) : CacheWorker(task_, db_) {}

void CacheWriter::exec() {
    if (task.is_cached)
        return;
    const std::string key = build_key();
    const std::string result = std::to_string(task.result);
    db.write(key, result, "0");
    Logger::getInstance().info("cache write: " + key + " = " + result);
}

// --- Runner ---
Runner::Runner()
    : db(),
      task(),
      parser(task),
      checker(task),
      cache_reader(task, db),
      calc(task),
      cache_writer(task, db),
      printer(task) {}

int Runner::run(int argc, char **argv) {
    if (argc < 2) {
        Logger::getInstance().error("Usage: iusearchbtw '<json_string>'");
        return 1;
    }

    try {
        parser.exec(argv[1]);
        checker.exec();
        cache_reader.exec();
        if (!task.is_cached) {
            calc.exec();
            cache_writer.exec();
        }
        printer.exec();
    } catch (const std::exception &e) {
        Logger::getInstance().error(std::string("error: ") + e.what());
        return 1;
    }

    return 0;
}
