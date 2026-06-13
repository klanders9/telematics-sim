// Copyright 2026 Kevin Landers <kmlanders@gmail.com>

#pragma once

#include "telemetry/i_mqtt_client.hpp"

#include <mosquitto.h>

class MosquittoClient : public IMqttClient {
public:
    explicit MosquittoClient(const std::string& client_id);
    ~MosquittoClient() override;

    bool connect(const std::string& host, int port, int keepalive = 60) override;
    bool publish(const std::string& topic, const std::string& payload, int qos = 1) override;
    bool subscribe(const std::string& topic, int qos = 1) override;
    bool loop_start() override;
    bool loop_stop() override;
    void set_message_handler(MessageHandler handler) override;
    void set_connect_handler(ConnectHandler handler) override;
    void set_disconnect_handler(DisconnectHandler handler) override;

private:
    static void on_message_static(struct mosquitto*, void* obj,
                                   const struct mosquitto_message* msg);
    static void on_connect_static(struct mosquitto*, void* obj, int rc);
    static void on_disconnect_static(struct mosquitto*, void* obj, int rc);

    void on_message(const struct mosquitto_message* msg);
    void on_connect(int rc);
    void on_disconnect(int rc);

    struct mosquitto* mosq_;
    MessageHandler    message_handler_;
    ConnectHandler    connect_handler_;
    DisconnectHandler disconnect_handler_;
};
