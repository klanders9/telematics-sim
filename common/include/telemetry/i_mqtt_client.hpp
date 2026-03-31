// Copyright 2026 Kevin Landers <kmlanders@gmail.com>

#pragma once

#include <functional>
#include <string>

// Pure virtual interface for MQTT client backends.
// Implementations: MosquittoClient (libmosquitto), AsyncMqttClient (async_mqtt).
//
// Threading contract: all set_*_handler() calls must happen before loop_start().
class IMqttClient {
public:
    using MessageHandler    = std::function<void(const std::string& topic,
                                                  const std::string& payload)>;
    using ConnectHandler    = std::function<void(int rc)>;  // rc==0 on success
    using DisconnectHandler = std::function<void(int rc)>;  // rc!=0 if unexpected

    virtual ~IMqttClient() = default;

    IMqttClient(IMqttClient const&) = delete;
    IMqttClient(IMqttClient&&) = delete;
    IMqttClient& operator=(IMqttClient const&) = delete;
    IMqttClient& operator=(IMqttClient&&) = delete;

    virtual bool connect(const std::string& host, int port, int keepalive = 60) = 0;
    virtual bool publish(const std::string& topic, const std::string& payload, int qos = 1) = 0;
    virtual bool subscribe(const std::string& topic, int qos = 1) = 0;

    // Starts the background event loop. Returns false if the thread cannot start
    // (e.g. MOSQ_ERR_NOT_SUPPORTED, or Asio executor unavailable).
    virtual bool loop_start() = 0;
    virtual bool loop_stop() = 0;

    virtual void set_message_handler(MessageHandler handler) = 0;
    virtual void set_connect_handler(ConnectHandler handler) = 0;
    virtual void set_disconnect_handler(DisconnectHandler handler) = 0;

protected:
    IMqttClient() = default;
};
