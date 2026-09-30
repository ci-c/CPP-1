#include "signal_watcher.hpp"

#include <pthread.h>
#include <signal.h>

SignalWatcher::SignalWatcher() {
    sigemptyset(&watch_set_);
    sigaddset(&watch_set_, SIGTERM);
    sigaddset(&watch_set_, SIGINT);
    sigaddset(&watch_set_, SIGHUP);
}

SignalWatcher::~SignalWatcher() { stop(); }

void SignalWatcher::start() {
    pthread_sigmask(SIG_BLOCK, &watch_set_, &saved_mask_);
    running_ = true;
    thread_ = std::thread(&SignalWatcher::loop, this);
}

void SignalWatcher::stop() {
    shutdown_ = true;
    if (!thread_.joinable()) {
        return;
    }
    running_ = false;
    pthread_kill(thread_.native_handle(), SIGTERM);
    thread_.join();
    pthread_sigmask(SIG_SETMASK, &saved_mask_, nullptr);
}

bool SignalWatcher::shutdown_requested() const { return shutdown_.load(); }

bool SignalWatcher::consume_reload_request() { return reload_.exchange(false); }

void SignalWatcher::loop() {
    int sig = 0;
    while (running_) {
        if (sigwait(&watch_set_, &sig) != 0) {
            continue;
        }
        if (sig == SIGTERM || sig == SIGINT) {
            shutdown_ = true;
            running_ = false;
        } else if (sig == SIGHUP) {
            reload_ = true;
        }
    }
}
