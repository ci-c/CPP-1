#include "runner.hpp"

#include <csignal>
#include <string>

int main(int argc, char **argv) {
    struct sigaction sa{};
    sa.sa_handler = SIG_IGN;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGPIPE, &sa, nullptr);

    if (argc == 2 && std::string(argv[1]) == "--daemon") {
        return Runner().run_service();
    }

    Runner runner;
    return runner.run(argc, argv);
}
