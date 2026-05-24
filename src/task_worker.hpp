#pragma once

#include "task.hpp"

class TaskWorker {
public:
    TaskWorker(Task &task_);
    virtual ~TaskWorker() = default;
    Task &getResult();

protected:
    Task &task;
};
