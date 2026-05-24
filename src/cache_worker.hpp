#pragma once

#include "db.hpp"
#include "task_worker.hpp"

#include <string>

class CacheWorker : public TaskWorker {
public:
    CacheWorker(Task &task_, Database &db_);

protected:
    Database &db;
    std::string build_key() const;
};
