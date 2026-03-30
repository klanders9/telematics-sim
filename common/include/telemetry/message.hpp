// Copyright 2026 Kevin Landers <kmlanders@gmail.com>

#ifndef TELEMATICS_SIM_MESSAGE_HPP
#define TELEMATICS_SIM_MESSAGE_HPP

#include <string>
#include <optional>
#include <cstdint>

struct GPS {
    double lat;
    double lon;
};

struct TelemetryMessage {
    std::string device_id;
    uint64_t timestamp;
    int rpm;
    int coolant_temp;
    int fuel_level;
    GPS gps;
    std::optional<std::string> fault;
};

#endif //TELEMATICS_SIM_MESSAGE_HPP
