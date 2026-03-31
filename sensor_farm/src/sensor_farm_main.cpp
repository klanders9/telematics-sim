// Copyright 2026 Kevin Landers <kmlanders@gmail.com>

#include <atomic>
#include <chrono>
#include <csignal>
#include <iostream>
#include <random>
#include <thread>

#include "telemetry/message.hpp"
#include "telemetry/json_utils.hpp"
#include "telemetry/i_mqtt_client.hpp"
#include "telemetry/mqtt_factory.hpp"

constexpr const char* PUB_TOPIC = "telematics/veh_001";
constexpr const char* CMD_TOPIC = "cmd/veh_001";

static std::atomic<bool> running{true};

TelemetryMessage generate_message(std::mt19937& rng) {
    TelemetryMessage msg;
    msg.device_id = "veh_001";
    msg.timestamp = std::chrono::system_clock::now().time_since_epoch().count();
    msg.rpm = 1800 + std::uniform_int_distribution<int>(0, 799)(rng);
    msg.coolant_temp = 80 + std::uniform_int_distribution<int>(0, 49)(rng);
    msg.gps = {34.25 + std::uniform_int_distribution<int>(0, 99)(rng) / 1000.0,
               -109.75 + std::uniform_int_distribution<int>(0, 99)(rng) / 1000.0};

    if (std::uniform_int_distribution<int>(0, 19)(rng) == 0) {
        msg.fault = "OVERHEAT";
    }
    else {
        msg.fault = std::nullopt;
    }

    return msg;
}

int main() {
    std::signal(SIGINT,  [](int) { running = false; });
    std::signal(SIGTERM, [](int) { running = false; });

    std::mt19937 rng(std::random_device{}());

    auto client = make_mqtt_client("sensor_farm");

    // Future: handle simulated OTA update request
    client->set_message_handler(
        [](const std::string& topic, const std::string& payload) {
            std::cout << "[SENSOR_FARM RX] " << topic << std::endl;
            std::cout << payload << std::endl;

            if (payload.find("update") != std::string::npos) {
                std::cout << "[SENSOR_FARM] Simulating OTA update..." << std::endl;
            }
        });

    // Subscribe inside on_connect so topics are re-subscribed after reconnect
    client->set_connect_handler(
        [&client](int rc) {
            if (rc != 0) return;
            std::cout << "[SENSOR_FARM] Connected to broker" << std::endl;
            client->subscribe(CMD_TOPIC, 1);
        });

    client->set_disconnect_handler(
        [](int rc) {
            if (rc != 0) {
                std::cout << "[SENSOR_FARM] Disconnected; will reconnect..." << std::endl;
            }
        });

    if (!client->connect("tcu", 1883)) {
        std::cerr << "Device failed to connect to broker" << std::endl;
        return 1;
    }

    if (!client->loop_start()) {
        std::cerr << "[SENSOR_FARM] Failed to start event loop" << std::endl;
        return 1;
    }

    std::cout << "[SENSOR_FARM]: Running... " << std::endl;

    while (running) {
        auto msg = generate_message(rng);
        std::string payload = to_json(msg);

        if (!client->publish(PUB_TOPIC, payload, 1)) {
            std::cerr << "[SENSOR_FARM] Failed to publish message" << std::endl;
        }
        else {
            std::cout << "[SENSOR_FARM TX] " << payload << std::endl;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }

    std::cout << "[SENSOR_FARM] Shutting down..." << std::endl;
    client->loop_stop();

    return 0;
}
