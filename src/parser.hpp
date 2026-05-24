#pragma once

#include "task_worker.hpp"

#include <string>

class Parser : public TaskWorker {
public:
    Parser(Task &task_);
    void exec(const std::string &input);
};
