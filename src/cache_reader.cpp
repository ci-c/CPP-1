#include "cache_reader.hpp"

#include "logger.hpp"

CacheReader::CacheReader(Task &task_, Database &db_) : CacheWorker(task_, db_) {}

void CacheReader::exec() {
    const std::string key = build_key();
    auto cached = db.read(key);
    if (cached) {
        task.result = std::stoi(*cached);
        task.is_cached = true;
        Logger::getInstance().info("cache hit: " + key + " = " + *cached);
    } else {
        Logger::getInstance().info("cache miss: " + key);
    }
}
