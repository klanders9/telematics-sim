// Copyright 2026 Kevin Landers <kmlanders@gmail.com>

#ifndef TELEMATICS_SIM_MQTT_CLIENT_HPP
#define TELEMATICS_SIM_MQTT_CLIENT_HPP

#include <mosquitto.h>
#include <functional>
#include <string>

class MqttClient {
public:
    using MessageHandler    = std::function<void(const std::string&, const std::string&)>;
    // rc == 0 on success; non-zero values are CONNACK reason codes
    using ConnectHandler    = std::function<void(int rc)>;
    using DisconnectHandler = std::function<void(int rc)>;

    explicit MqttClient(const std::string& client_id);
    ~MqttClient();

    MqttClient(MqttClient const&) = delete;
    MqttClient(MqttClient&&) = delete;
    MqttClient& operator=(MqttClient const&) = delete;
    MqttClient& operator=(MqttClient&&) = delete;

    bool connect(const std::string& host, int port, int keepalive = 60);
    bool publish(const std::string& topic, const std::string& payload, int qos = 1);
    bool subscribe(const std::string& topic, int qos = 1);

    // Returns false if the background thread could not be started
    // (e.g. MOSQ_ERR_NOT_SUPPORTED on a platform without thread support).
    bool loop_start();
    bool loop_stop();

    // All handlers must be set before loop_start() to avoid data races.
    void set_message_handler(MessageHandler handler);
    void set_connect_handler(ConnectHandler handler);
    void set_disconnect_handler(DisconnectHandler handler);

private:
    static void on_message_static(struct mosquitto*, void* obj, const struct mosquitto_message* msg);
    static void on_connect_static(struct mosquitto*, void* obj, int rc);
    static void on_disconnect_static(struct mosquitto*, void* obj, int rc);

    void on_message(const struct mosquitto_message* msg);
    void on_connect(int rc);
    void on_disconnect(int rc);

    struct mosquitto* mosq_;
    MessageHandler    handler_;
    ConnectHandler    connect_handler_;
    DisconnectHandler disconnect_handler_;
};

#endif //TELEMATICS_SIM_MQTT_CLIENT_HPP
