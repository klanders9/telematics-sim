// Copyright 2026 Kevin Landers <kmlanders@gmail.com>

#include <chrono>
#include <iostream>
#include <thread>

#include "telemetry/mqtt_client.hpp"

constexpr const char* SUB_TOPIC = "cloud/#";

int main() {
    MqttClient client{"backend"};

    if (!client.connect("tcu", 1883, 60)) {
        std::cerr << "Backend connect failed" << std::endl;
        return 1;
    }

    client.set_message_handler(
        [](const std::string& topic, const std::string& payload) {
            std::cout << "[BACKEND] " << topic << std::endl;
            std::cout << payload << std::endl;

            if (payload.find("OVERHEAT") != std::string::npos) {
                std::cout << "[BACKEND] !!! ALERT: Overheat detected !!!" << std::endl;
            }
        });

    client.subscribe(SUB_TOPIC, 1);
    client.loop_start();

    while (true) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    client.loop_stop();

    return 0;
}