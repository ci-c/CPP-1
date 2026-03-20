#include <iostream>

enum class Operation {
    ADD,
    SUBTRACT,
    MULTIPLY,
    DIVIDE,
    POWER,
    FACTORIAL
};


#include <getopt.h>

int getopt_long(int argc, char * const argv[],
           const char *optstring,
           const struct option *longopts, int *longindex);


int main(int argc, char *argv[]) {

    int a = stoi2(argv[1]);
    int b = stoi2(argv[2]);
    Operation op = argv[3] ? static_cast<Operation>(stoi2(argv[3])) : Operation::ADD;

    return 0;
}

int safe_division(int &numerator, int denominator) {
    if (denominator == 0) {
        return 1;
    } else {
        numerator /= denominator;
        return 0;
    }
}

int calculate(int &a, int &b, int &c) {}

void print_help() {}


int stoi2(const char *str) {
    int i = 0;
    int result = 0;
    bool negative = false;
    while (str[i] == '\0') {
        if (str[i] == '-') {
            negative = true;
        } else if (str[i] >= '0' && str[i] <= '9') {
            result = result * 10 + (str[i] - '0');
        } else {
            break;
        }
        i++;
    }
    if (negative) {
        result = -result;
    }
    return result;
}
