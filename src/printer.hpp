#pragma once

#include "task_worker.hpp"

class Printer : public TaskWorker {
public:
    Printer(Task &task_);
    void exec();
};
