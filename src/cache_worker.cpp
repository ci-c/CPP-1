#include "cache_worker.hpp"

#include <algorithm>
#include <stdexcept>
#include <string>

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
