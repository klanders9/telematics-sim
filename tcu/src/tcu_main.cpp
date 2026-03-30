// Copyright 2026 Kevin Landers <kmlanders@gmail.com>

#include <chrono>
#include <iostream>
#include <string>
#include <thread>

#include "telemetry/mqtt_client.hpp"

constexpr const char* DEVICE_TOPIC = "telematics/veh_001";
constexpr const char* CLOUD_TOPIC = "cloud/veh_001";

int main() {
    MqttClient client("tcu");

    if (!client.connect("localhost", 1883)) {
        std::cerr << "TCU failed to connect" << std::endl;
        return 1;
    }

    client.set_message_handler(
        [&](const std::string& topic, const std::string& payload) {
            std::cout << "[TCU] RX: " << topic << std::endl;

            // Update payload
            std::string enriched = payload;

            // Example: add a tag for TCU
            enriched.insert(enriched.size() - 1, ",\"tcu\":\"pi4\"");

            // Forward to cloud namespace
            client.publish(CLOUD_TOPIC, enriched, 1);
        });

    client.subscribe(DEVICE_TOPIC, 1);
    client.loop_start();

    std::cout << "[TCU] running..." << std::endl;

    while (true) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    client.loop_stop();
    std::cout << "[TCU] stopped..." << std::endl;

    return 0;
}