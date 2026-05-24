#include "task_worker.hpp"

TaskWorker::TaskWorker(Task &task_) : task(task_) {}

Task &TaskWorker::getResult() { return task; }
