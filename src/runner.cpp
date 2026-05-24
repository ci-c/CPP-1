#include "runner.hpp"

#include "logger.hpp"

#include <exception>

Runner::Runner()
    : db(),
      task(),
      parser(task),
      checker(task),
      cache_reader(task, db),
      calc(task),
      cache_writer(task, db),
      printer(task) {}

int Runner::run(int argc, char **argv) {
    if (argc < 2) {
        Logger::getInstance().error("Usage: iusearchbtw '<json_string>'");
        return 1;
    }

    try {
        parser.exec(argv[1]);
        checker.exec();
        cache_reader.exec();
        if (!task.is_cached) {
            calc.exec();
            cache_writer.exec();
        }
        printer.exec();
    } catch (const std::exception &e) {
        Logger::getInstance().error(std::string("error: ") + e.what());
        return 1;
    }

    return 0;
}
