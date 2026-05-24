#pragma once

#include "cache_worker.hpp"

class CacheReader : public CacheWorker {
public:
    CacheReader(Task &task_, Database &db_);
    void exec();
};
