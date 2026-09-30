#pragma once

#include <memory>
#include <spdlog/logger.h>
#include <string>

class Logger {
  public:
    static Logger &getInstance();
    static void set_info_level();

    Logger(const Logger &) = delete;
    Logger &operator=(const Logger &) = delete;

    void info(const std::string &message);
    void error(const std::string &message);

  private:
    Logger();

    std::shared_ptr<spdlog::logger> spd_logger;
};
