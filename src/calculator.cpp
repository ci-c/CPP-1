#include "calculator.hpp"

#include "math_ops.hpp"

#include <stdexcept>

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
