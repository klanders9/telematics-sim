// Copyright 2026 Kevin Landers <kmlanders@gmail.com>

#include "telemetry/json_utils.hpp"
#include <sstream>

#include "../include/telemetry/message.hpp"

std::string to_json(const TelemetryMessage& msg) {
    std::ostringstream oss;
    oss << "{";
    oss << "\"device_id\":\"" << msg.device_id << "\",";
    oss << "\"timestamp\":" << msg.timestamp << ",";
    oss << "\"rpm\":" << msg.rpm << ",";
    oss << "\"coolant_temp\":" << msg.coolant_temp << ",";
    oss << "\"fuel_level\":" << msg.fuel_level << ",";
    oss << "\"gps\":{";
    oss << "\"lat\":" << msg.gps.lat << ",";
    oss << "\"lon\":" << msg.gps.lon << "},";

    if (msg.fault.has_value()) {
        oss << "\"fault\":" << msg.fault.value() << "\"";
    }
    else {
        oss << "\"fault\":null";
    }

    oss << "}";
    return oss.str();
}
