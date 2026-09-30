#pragma once

#include <atomic>
#include <csignal>
#include <thread>

class SignalWatcher {
  public:
    SignalWatcher();
    ~SignalWatcher();
    SignalWatcher(const SignalWatcher &) = delete;
    SignalWatcher &operator=(const SignalWatcher &) = delete;
    void start();
    void stop();
    bool shutdown_requested() const;
    bool consume_reload_request();

  private:
    void loop();
    sigset_t watch_set_{};
    sigset_t saved_mask_{};
    std::thread thread_;
    std::atomic<bool> shutdown_{false};
    std::atomic<bool> reload_{false};
    std::atomic<bool> running_{false};
};
