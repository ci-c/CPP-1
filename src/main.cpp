#include <getopt.h>
#include <stdio.h>
#include "math_ops.hpp"

enum TaskState {
    TASK_OK = 0,
    TASK_HELP = 1,
    TASK_ERROR = -1
};

struct Task {
    int first_number;
    int second_number;
    char operation;
    double result;
    int state;
};



int run(int argc, char **argv);
int parse(int argc, char **argv, Task &task);
int check(Task &task);
int calculate(Task &task);
int print(Task &task);
void print_help();
int stoi2(const char *str);

int main(int argc, char **argv) {
    return run(argc, argv);
}

int run(int argc, char **argv) {
    Task task;
    task.first_number = 0;
    task.second_number = 0;
    task.operation = 0;
    task.result = 0;
    task.state = TASK_OK;

    parse(argc, argv, task);
    if (task.state != TASK_OK) {
        return print(task);
    }

    check(task);
    if (task.state != TASK_OK) {
        return print(task);
    }

    calculate(task);
    return print(task);
}

int parse(int argc, char **argv, Task &task) {
    static struct option long_options[] = {
        {"help", 0, 0, 'h'},
        {0, 0, 0, 0}
    };

    int option_char = 0;

    while ((option_char = getopt_long(argc, argv, "h", long_options, 0)) != -1) {
        if (option_char == 'h') {
            task.state = TASK_HELP;
            return task.state;
        }

        task.state = TASK_ERROR;
        return task.state;
    }

    if (argc - optind == 3) {
        task.first_number = stoi2(argv[optind]);
        task.operation = argv[optind + 1][0];
        task.second_number = stoi2(argv[optind + 2]);
        task.state = TASK_OK;
        return task.state;
    }

    task.state = TASK_ERROR;
    return task.state;
}

int check(Task &task) {
    if (task.operation == '/' && task.second_number == 0) {
        task.state = TASK_ERROR;
        return task.state;
    }

    task.state = TASK_OK;
    return task.state;
}

int calculate(Task &task) {
    int math_result = 0;

    if (task.state != TASK_OK) {
        return task.state;
    }

    switch (task.operation) {
        case '+':
            task.state = mathlib::add(task.first_number, task.second_number, &math_result);
            break;
        case '-':
            task.state = mathlib::sub(task.first_number, task.second_number, &math_result);
            break;
        case '*':
            task.state = mathlib::mul(task.first_number, task.second_number, &math_result);
            break;
        case '/':
            task.state = mathlib::divide(task.first_number, task.second_number, &math_result);
            break;
        case '^':
            task.state = mathlib::power(task.first_number, task.second_number, &math_result);
            break;
        case '!':
            task.state = mathlib::factorial(task.first_number, &math_result);
            break;
        default:
            task.state = TASK_ERROR;
            return task.state;
    }

    if (task.state == TASK_OK) {
        task.result = math_result;
    }

    return task.state;
}
int print(Task &task) {
    if (task.state == TASK_HELP) {
        print_help();
        return 0;
    }

    if (task.state == TASK_ERROR) {
        printf("error\n");
        return 1;
    }

    printf("%f\n", task.result);
    return 0;
}

void print_help() {
    printf("Usage: calculator <a> <op> <b>\n");
    printf("Example: calculator 10 + 20\n");
}

int stoi2(const char *str) {
    int index = 0;
    int result = 0;
    bool is_negative = false;

    while (true) {
        if (str[index] == '-') {
            is_negative = true;
        } else if (str[index] >= '0' && str[index] <= '9') {
            result = result * 10 + (str[index] - '0');
        } else {
            break;
        }

        index++;
    }

    if (is_negative) {
        result = -result;
    }

    return result;
}
