// Copyright 2026 Kevin Landers <kmlanders@gmail.com>

#ifndef TELEMATICS_SIM_MQTT_CLIENT_HPP
#define TELEMATICS_SIM_MQTT_CLIENT_HPP

#include <mosquitto.h>
#include <functional>
#include <string>

class MqttClient {
public:
    using MessageHandler = std::function<void(const std::string&, const std::string&)>;

    MqttClient(const std::string& client_id);
    ~MqttClient();

    bool connect(const std::string& host, int port, int keepalive = 60);
    bool publish(const std::string& topic, const std::string& payload, int qos = 1);
    bool subscribe(const std::string& topic, int qos = 1);

    void loop_start();
    void loop_stop();

    void set_message_handler(MessageHandler handler);

private:
    static void on_message_static(struct mosquitto* mosq, void* userdata, const struct mosquitto_message* msg);

    void on_message(const struct mosquitto_message* msg);

    struct mosquitto* mosq_;
    MessageHandler handler_;
};

#endif //TELEMATICS_SIM_MQTT_CLIENT_HPP
