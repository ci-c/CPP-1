#include "app.hpp"
#include <iostream>
#include <stdio.h>

// --- TaskWorker ---
TaskWorker::TaskWorker(Task &task_) : task{task_} {}
Task& TaskWorker::getResult() { return task; }

// --- Calculator ---
Calculator::Calculator(Task &task_) : TaskWorker(task_) {}

void Calculator::exec() {
    int math_result = 0;
    switch (task.operation) {
    case '+': mathlib::add(task.first_number, task.second_number, &math_result); break;
    case '-': mathlib::sub(task.first_number, task.second_number, &math_result); break;
    case '*': mathlib::mul(task.first_number, task.second_number, &math_result); break;
    case '/': mathlib::divide(task.first_number, task.second_number, &math_result); break;
    case '^': mathlib::power(task.first_number, task.second_number, &math_result); break;
    case '!': mathlib::factorial(task.first_number, &math_result); break;
    default: throw std::runtime_error("Invalid operation");
    }
    task.result = math_result;
}

// --- Parser ---
Parser::Parser(Task &task_, const std::string& input) : TaskWorker(task_), json_str(input) {}

void Parser::exec() {
    using json = nlohmann::json;
    json json_input = json::parse(json_str); 
    
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
    case '+': case '-': case '*': case '/': case '^': return true;
    default: return false;
    }
}

// --- Printer ---
Printer::Printer(Task &task_) : TaskWorker(task_) {}

void Printer::exec() {
    printf("%d\n", task.result);
}

// --- Runner ---
int Runner::run(int argc, char **argv) {
    if (argc < 2) {
        Logger::getInstance().error("Usage: iusearchbtw '<json_string>'");
        return 1;
    }

    Task task;
    Parser parser(task, argv[1]);
    Checker checker(task);
    Calculator calc(task);
    Printer printer(task);

    try {
        parser.exec();
        checker.exec();
        calc.exec();
        printer.exec();
    } catch (const std::exception &e) {
        Logger::getInstance().error(std::string("error: ") + e.what());
        return 1;
    }

    return 0;
}