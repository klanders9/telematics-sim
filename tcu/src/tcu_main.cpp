// Copyright 2026 Kevin Landers <kmlanders@gmail.com>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <csignal>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>

#include "telemetry/mqtt_client.hpp"

constexpr const char* DEVICE_TOPIC = "telematics/#";
constexpr const char* CLOUD_TOPIC  = "cloud/veh_001";

static std::atomic<bool>     running{true};
static std::mutex             shutdown_mutex;
static std::condition_variable shutdown_cv;

int main() {
    std::signal(SIGINT,  [](int) { running = false; shutdown_cv.notify_all(); });
    std::signal(SIGTERM, [](int) { running = false; shutdown_cv.notify_all(); });

    MqttClient client("tcu");

    client.set_message_handler(
        [&](const std::string& topic, const std::string& payload) {
            std::cout << "[TCU RX] " << topic << std::endl;

            // Enrich payload: insert tcu tag before closing brace
            std::string enriched = payload;
            auto pos = enriched.rfind('}');
            if (pos != std::string::npos) {
                enriched.insert(pos, ",\"tcu\":\"pi4\"");
            }

            client.publish(CLOUD_TOPIC, enriched, 1);
        });

    // Subscribe inside on_connect so topics are re-subscribed after reconnect
    client.set_connect_handler(
        [&client](int rc) {
            if (rc != 0) return;
            std::cout << "[TCU] Connected to broker" << std::endl;
            client.subscribe(DEVICE_TOPIC, 1);
        });

    client.set_disconnect_handler(
        [](int rc) {
            if (rc != 0) {
                std::cout << "[TCU] Disconnected; will reconnect..." << std::endl;
            }
        });

    if (!client.connect("tcu", 1883)) {
        std::cerr << "TCU failed to connect" << std::endl;
        return 1;
    }

    if (!client.loop_start()) {
        std::cerr << "[TCU] Failed to start event loop" << std::endl;
        return 1;
    }

    std::cout << "[TCU] running..." << std::endl;

    // Block until signal rather than spinning
    std::unique_lock<std::mutex> lock(shutdown_mutex);
    shutdown_cv.wait(lock, [] { return !running.load(); });

    std::cout << "[TCU] stopped..." << std::endl;
    client.loop_stop();

    return 0;
}
