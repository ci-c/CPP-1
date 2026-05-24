#include "printer.hpp"

#include <stdio.h>

Printer::Printer(Task &task_) : TaskWorker(task_) {}

void Printer::exec() { printf("%d\n", task.result); }
