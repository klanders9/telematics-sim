// Copyright 2026 Kevin Landers <kmlanders@gmail.com>

#include <iostream>
#include <thread>
#include <chrono>
#include <cstdlib>

#include "telemetry/message.hpp"
#include "telemetry/json_utils.hpp"
#include "telemetry/mqtt_client.hpp"

constexpr const char* PUB_TOPIC = "telematics/veh_001";
constexpr const char* CMD_TOPIC = "cmd/veh_001";

TelemetryMessage generate_message() {
    TelemetryMessage msg;
    msg.device_id = "veh_001";
    msg.timestamp = std::chrono::system_clock::now().time_since_epoch().count();
    msg.rpm = 1800 + rand() % 800;
    msg.coolant_temp = 80 + rand() % 50;
    msg.gps = {34.25 + (rand() % 100) / 1000.0, -109.75 + (rand() % 100) / 1000.0};

    // simulate occasional fault
    if (rand() % 20 == 0) {
        msg.fault = "OVERHEAT";
    }
    else {
        msg.fault = std::nullopt;
    }

    return msg;
}

int main() {
    srand(time(nullptr));

    MqttClient client("sensor_farm");
    if (!client.connect("tcu", 1883)) {
        std::cerr << "Device failed to connect to broker" << std::endl;
        return 1;
    }

    // Future: handle simualted OTA update request
    client.set_message_handler(
        [](const std::string& topic, const std::string& payload) {
            std::cout << "[SENSOR_FARM RX] " << topic << std::endl;
            std::cout << payload << std::endl;

            if (payload.find("update") != std::string::npos) {
                std::cout << "[SENSOR_FARM] Simulating OTA update..." << std::endl;
            }
        });

    client.subscribe(CMD_TOPIC, 1);
    client.loop_start();

    std::cout << "[SENSOR_FARM]: Running... " << std::endl;

    while (true) {
        auto msg = generate_message();
        std::string payload = to_json(msg);

        if (!client.publish(PUB_TOPIC, payload, 1)) {
            std::cerr << "[SENSOR_FARM] Failed to publish message" << std::endl;
        }
        else {
            std::cout << "[SENSOR_FARM TX] :" << payload << std::endl;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }

    client.loop_stop();

    return 0;

}