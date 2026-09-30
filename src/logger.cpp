#include "logger.hpp"

#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

Logger &Logger::getInstance() {
    static Logger instance;
    return instance;
}

Logger::Logger() {
    spd_logger = spdlog::stdout_color_mt("console");
    spdlog::set_level(spdlog::level::err);
}

void Logger::info(const std::string &message) { spd_logger->info(message); }

void Logger::error(const std::string &message) { spd_logger->error(message); }
// spdlog::set_level меняет уровень default-логгера и всех логгеров реестра,
// поэтому на уровень экземпляра надо действовать напрямую
void Logger::set_info_level() { getInstance().spd_logger->set_level(spdlog::level::info); }
