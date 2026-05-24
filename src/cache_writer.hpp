#pragma once

#include "cache_worker.hpp"

class CacheWriter : public CacheWorker {
public:
    CacheWriter(Task &task_, Database &db_);
    void exec();
};
