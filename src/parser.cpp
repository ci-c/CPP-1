#include "parser.hpp"

#include <nlohmann/json.hpp>

Parser::Parser(Task &task_) : TaskWorker(task_) {}

void Parser::exec(const std::string &input) {
    using json = nlohmann::json;
    json json_input = json::parse(input);

    std::string op_str = json_input["operation"].get<std::string>();
    task.operation = op_str[0];

    task.first_number = json_input["first_number"].get<int>();
    if (json_input.contains("second_number")) {
        task.second_number = json_input["second_number"].get<int>();
    }
}
