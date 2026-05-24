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
