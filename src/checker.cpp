#include "checker.hpp"

#include <stdexcept>

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
