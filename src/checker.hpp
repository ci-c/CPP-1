#pragma once

#include "task_worker.hpp"

class Checker : public TaskWorker {
public:
    Checker(Task &task_);
    void exec();

private:
    bool is_binary_operation(char operation) const;
};
