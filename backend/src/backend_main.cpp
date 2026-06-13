// Copyright 2026 Kevin Landers <kmlanders@gmail.com>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <csignal>
#include <iostream>
#include <mutex>
#include <thread>

#include "telemetry/i_mqtt_client.hpp"
#include "telemetry/mqtt_factory.hpp"

constexpr const char* SUB_TOPIC = "cloud/#";

static std::atomic<bool>      running{true};
static std::mutex              shutdown_mutex;
static std::condition_variable shutdown_cv;

int main() {
    std::signal(SIGINT,  [](int) { running = false; shutdown_cv.notify_all(); });
    std::signal(SIGTERM, [](int) { running = false; shutdown_cv.notify_all(); });

    auto client = make_mqtt_client("backend");

    client->set_message_handler(
        [](const std::string& topic, const std::string& payload) {
            std::cout << "[BACKEND] " << topic << std::endl;
            std::cout << payload << std::endl;

            if (payload.find("OVERHEAT") != std::string::npos) {
                std::cout << "[BACKEND] !!! ALERT: Overheat detected !!!" << std::endl;
            }
        });

    client->set_connect_handler(
        [&client](int rc) {
            if (rc != 0) return;
            std::cout << "[BACKEND] Connected to broker" << std::endl;
            client->subscribe(SUB_TOPIC, 1);
        });

    client->set_disconnect_handler(
        [](int rc) {
            if (rc != 0) {
                std::cout << "[BACKEND] Disconnected; will reconnect..." << std::endl;
            }
        });

    if (!client->connect("tcu", 1883, 60)) {
        std::cerr << "Backend connect failed" << std::endl;
        return 1;
    }

    if (!client->loop_start()) {
        std::cerr << "[BACKEND] Failed to start event loop" << std::endl;
        return 1;
    }

    std::cout << "[BACKEND] running..." << std::endl;

    std::unique_lock<std::mutex> lock(shutdown_mutex);
    shutdown_cv.wait(lock, [] { return !running.load(); });

    std::cout << "[BACKEND] stopped..." << std::endl;
    client->loop_stop();

    return 0;
}
