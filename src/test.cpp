#include <gtest/gtest.h>
#include <nlohmann/json.hpp>
#include <string>
#include <stdexcept>
#include "app.hpp"

class CalculatorTest : public ::testing::Test {
protected:
    Task task;
};

TEST_F(CalculatorTest, Addition) {
    task.first_number = 5;
    task.second_number = 3;
    task.operation = '+';

    Calculator calc(task);
    calc.exec();

    EXPECT_EQ(task.result, 8);
}

TEST_F(CalculatorTest, Subtraction) {
    task.first_number = 10;
    task.second_number = 4;
    task.operation = '-';

    Calculator calc(task);
    calc.exec();

    EXPECT_EQ(task.result, 6);
}

TEST_F(CalculatorTest, Multiplication) {
    task.first_number = 7;
    task.second_number = 6;
    task.operation = '*';

    Calculator calc(task);
    calc.exec();

    EXPECT_EQ(task.result, 42);
}

TEST_F(CalculatorTest, Division) {
    task.first_number = 20;
    task.second_number = 4;
    task.operation = '/';

    Calculator calc(task);
    calc.exec();

    EXPECT_EQ(task.result, 5);
}

TEST_F(CalculatorTest, Power) {
    task.first_number = 2;
    task.second_number = 3;
    task.operation = '^';

    Calculator calc(task);
    calc.exec();

    EXPECT_EQ(task.result, 8);
}

TEST_F(CalculatorTest, Factorial) {
    task.first_number = 5;
    task.operation = '!';

    Calculator calc(task);
    calc.exec();

    EXPECT_EQ(task.result, 120);
}

TEST_F(CalculatorTest, InvalidOperation) {
    task.operation = '?';

    Calculator calc(task);
    EXPECT_THROW(calc.exec(), std::runtime_error);
}

class ParserTest : public ::testing::Test {
protected:
    Task task;
};

TEST_F(ParserTest, ValidJsonAddition) {
    std::string json_str = R"({"operation": "+", "first_number": 5, "second_number": 3})";

    Parser parser(task, json_str);
    parser.exec();

    EXPECT_EQ(task.operation, '+');
    EXPECT_EQ(task.first_number, 5);
    EXPECT_EQ(task.second_number, 3);
}

TEST_F(ParserTest, ValidJsonFactorial) {
    std::string json_str = R"({"operation": "!", "first_number": 4})";

    Parser parser(task, json_str);
    parser.exec();

    EXPECT_EQ(task.operation, '!');
    EXPECT_EQ(task.first_number, 4);
}

TEST_F(ParserTest, InvalidJson) {
    std::string json_str = R"(invalid json)";

    Parser parser(task, json_str);
    EXPECT_THROW(parser.exec(), nlohmann::json::parse_error);
}

class CheckerTest : public ::testing::Test {
protected:
    Task task;
};

TEST_F(CheckerTest, ValidAddition) {
    task.operation = '+';
    task.first_number = 5;
    task.second_number = 3;

    Checker checker(task);
    EXPECT_NO_THROW(checker.exec());
}

TEST_F(CheckerTest, DivisionByZero) {
    task.operation = '/';
    task.first_number = 10;
    task.second_number = 0;

    Checker checker(task);
    EXPECT_THROW(checker.exec(), std::invalid_argument);
}

TEST_F(CheckerTest, NegativePower) {
    task.operation = '^';
    task.first_number = 2;
    task.second_number = -1;

    Checker checker(task);
    EXPECT_THROW(checker.exec(), std::invalid_argument);
}

TEST_F(CheckerTest, NegativeFactorial) {
    task.operation = '!';
    task.first_number = -1;

    Checker checker(task);
    EXPECT_THROW(checker.exec(), std::invalid_argument);
}

TEST_F(CheckerTest, UnknownOperation) {
    task.operation = '?';

    Checker checker(task);
    EXPECT_THROW(checker.exec(), std::invalid_argument);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
} 