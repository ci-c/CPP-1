#pragma once

#include "task_worker.hpp"

class Calculator : public TaskWorker {
public:
    Calculator(Task &task_);
    void exec();
};
