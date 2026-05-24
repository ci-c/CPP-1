#include "cache_writer.hpp"

#include "logger.hpp"

CacheWriter::CacheWriter(Task &task_, Database &db_) : CacheWorker(task_, db_) {}

void CacheWriter::exec() {
    if (task.is_cached)
        return;
    const std::string key = build_key();
    const std::string result = std::to_string(task.result);
    db.write(key, result, "0");
    Logger::getInstance().info("cache write: " + key + " = " + result);
}
