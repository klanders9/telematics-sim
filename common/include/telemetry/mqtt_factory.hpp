// Copyright 2026 Kevin Landers <kmlanders@gmail.com>

#pragma once

#include "telemetry/i_mqtt_client.hpp"

#include <memory>
#include <string>

#ifdef USE_ASYNC_MQTT
#  include "telemetry/async_mqtt_client.hpp"
inline std::unique_ptr<IMqttClient> make_mqtt_client(const std::string& client_id) {
    return std::make_unique<AsyncMqttClient>(client_id);
}
#else
#  include "telemetry/mosquitto_client.hpp"
inline std::unique_ptr<IMqttClient> make_mqtt_client(const std::string& client_id) {
    return std::make_unique<MosquittoClient>(client_id);
}
#endif
