#pragma once

#include "calculator.hpp"
#include "cache_reader.hpp"
#include "cache_writer.hpp"
#include "checker.hpp"
#include "db.hpp"
#include "parser.hpp"
#include "printer.hpp"
#include "task.hpp"

class Runner {
public:
    Runner();
    int run(int argc, char **argv);

private:
    Database db;
    Task task;
    Parser parser;
    Checker checker;
    CacheReader cache_reader;
    Calculator calc;
    CacheWriter cache_writer;
    Printer printer;
};
