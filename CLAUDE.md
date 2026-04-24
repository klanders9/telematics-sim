# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

A C++17 MQTT-based distributed telematics simulation sandbox. Three executables communicate via MQTT through a broker chain:

```
sensor_farm → (MQTT: telematics/veh_001) → tcu → (MQTT: cloud/veh_001) → backend
```

The TCU acts as both an MQTT broker and a message enricher (adds `"tcu":"pi4"` to payloads). `sensor_farm` connects to hostname `"tcu"` on port 1883.

## Build

**Native (host machine):**
```bash
cmake -B build
cmake --build build
```
Requires: C++17 compiler, CMake 3.31+, libmosquitto (via pkg-config).

**Cross-compile for Raspberry Pi 4 (aarch64):**
```bash
cmake -B build-rpi4 \
  -DCMAKE_TOOLCHAIN_FILE=cmake/aarch64-rpi4.cmake \
  -DUSE_ASYNC_MQTT=ON
cmake --build build-rpi4
```
The toolchain file points at `/opt/rpi-tools/aarch64-rpi4-linux-gnu`. Use `USE_ASYNC_MQTT=ON` for cross-builds — mosquitto dev libs are not in the sysroot so the default mosquitto backend will fail to configure. The async_mqtt backend pulls all dependencies via FetchContent and has no host-library requirements.

Pre-built executables are in `build/backend/backend`, `build/tcu/tcu`, `build/sensor_farm/sensor_farm_sim`.

## Running

Start in this order (each in its own terminal):

```bash
./build/backend/backend
./build/tcu/tcu
./build/sensor_farm/sensor_farm_sim
```

## Architecture

**`common/`** — Shared library used by all three components:
- `include/telemetry/message.hpp` — `TelemetryMessage` struct (device_id, timestamp, rpm, coolant_temp, fuel_level, gps, fault)
- `include/telemetry/mqtt_client.hpp` — `MqttClient` class wrapping libmosquitto with a C++ callback interface (`set_message_handler`)
- `src/json_utils.cpp` — Manual JSON serialization (no external JSON library)

**`sensor_farm/`** — Publishes simulated vehicle telemetry to `telematics/veh_001` every second. Generates random RPM/temp/GPS, 5% chance of `fault="OVERHEAT"`. Subscribes to `cmd/veh_001` for future OTA simulation.

**`tcu/`** — Subscribes to `telematics/#`, enriches JSON by inserting `"tcu":"pi4"` before the closing brace, republishes to `cloud/veh_001`. Also runs the MQTT broker (listens on port 1883).

**`backend/`** — Subscribes to `cloud/#`, logs incoming telemetry, and prints an alert when `"OVERHEAT"` appears in the payload.

## Key Implementation Details

- `MqttClient` uses a static callback to bridge libmosquitto's C-style callbacks to C++ instance methods
- JSON is built via string concatenation in `json_utils.cpp` — no nlohmann/json or similar
- TCU message enrichment is a string insertion (finds last `}`, inserts before it) rather than parse/re-serialize
- Timestamps use nanoseconds from `std::chrono::system_clock`
