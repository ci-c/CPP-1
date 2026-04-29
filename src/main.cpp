#include "math_ops.hpp"
#include <getopt.h>
#include <limits.h>
#include <stdio.h>

enum TaskState { TASK_OK = 0, TASK_HELP = 1, TASK_ERROR = -1 };

struct Task {
    int first_number;
    int second_number;
    char operation;
    int result;
    int state;
};

int run(int argc, char **argv);
int parse(int argc, char **argv, Task &task);
int check(Task &task);
int calculate(Task &task);
int print(Task &task);
void print_help();
int stoi2(const char *str, int *result);
bool is_binary_operation(char operation);

int main(int argc, char **argv) { return run(argc, argv); }

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
    static struct option long_options[] = {{"help", 0, 0, 'h'}, {0, 0, 0, 0}};

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
        if (stoi2(argv[optind], &task.first_number) != TASK_OK) {
            task.state = TASK_ERROR;
            return task.state;
        }

        if (argv[optind + 1][0] == '\0' || argv[optind + 1][1] != '\0') {
            task.state = TASK_ERROR;
            return task.state;
        }

        task.operation = argv[optind + 1][0];

        if (stoi2(argv[optind + 2], &task.second_number) != TASK_OK) {
            task.state = TASK_ERROR;
            return task.state;
        }

        task.state = TASK_OK;
        return task.state;
    }

    task.state = TASK_ERROR;
    return task.state;
}

int check(Task &task) {
    if (task.state != TASK_OK) {
        return task.state;
    }

    if (is_binary_operation(task.operation)) {
        if (task.operation == '/' && task.second_number == 0) {
            task.state = TASK_ERROR;
            return task.state;
        }

        if (task.operation == '^' && task.second_number < 0) {
            task.state = TASK_ERROR;
            return task.state;
        }

        task.state = TASK_OK;
        return task.state;
    }

    if (task.operation == '!') {
        if (task.first_number < 0) {
            task.state = TASK_ERROR;
            return task.state;
        }

        task.state = TASK_OK;
        return task.state;
    }

    task.state = TASK_ERROR;
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

    printf("%d\n", task.result);
    return 0;
}

void print_help() {
    printf("Usage:\n");
    printf("  iusearchbtw [--help]\n");
    printf("  iusearchbtw <a> <op> <b>\n");
    printf("\n");
    printf("Operations:\n");
    printf("  +  addition\n");
    printf("  -  subtraction\n");
    printf("  *  multiplication\n");
    printf("  /  division\n");
    printf("  ^  power\n");
    printf("  !  factorial\n");
    printf("\n");
    printf("Notes:\n");
    printf("  For factorial, only the first number is used.\n");
    printf("  The third argument is accepted but ignored: <a> ! <b>\n");
    printf("  If a number starts with '-', use '--' before arguments.\n");
    printf("\n");
    printf("Examples:\n");
    printf("  iusearchbtw 10 + 20\n");
    printf("  iusearchbtw 2 ^ 5\n");
    printf("  iusearchbtw 5 ! 0\n");
    printf("  iusearchbtw -- -5 + 2\n");
}

int stoi2(const char *str, int *result) {
    int index = 0;
    bool is_negative = false;
    long long value = 0;

    if (str == nullptr || result == nullptr || str[0] == '\0') {
        return TASK_ERROR;
    }

    if (str[index] == '-') {
        is_negative = true;
        index++;
    } else if (str[index] == '+') {
        index++;
    }

    if (str[index] < '0' || str[index] > '9') {
        return TASK_ERROR;
    }

    while (str[index] >= '0' && str[index] <= '9') {
        value = value * 10 + (str[index] - '0');

        if ((!is_negative && value > INT_MAX) ||
            (is_negative && value > static_cast<long long>(INT_MAX) + 1)) {
            return TASK_ERROR;
        }

        index++;
    }

    if (str[index] != '\0') {
        return TASK_ERROR;
    }

    if (is_negative) {
        if (value == static_cast<long long>(INT_MAX) + 1) {
            *result = INT_MIN;
            return TASK_OK;
        }

        *result = -static_cast<int>(value);
        return TASK_OK;
    }

    *result = static_cast<int>(value);
    return TASK_OK;
}

bool is_binary_operation(char operation) {
    switch (operation) {
    case '+':
    case '-':
    case '*':
    case '/':
    case '^':
        return true;
    default:
        return false;
    }
}
