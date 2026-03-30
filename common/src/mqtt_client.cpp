// Copyright 2026 Kevin Landers <kmlanders@gmail.com>

#include "telemetry/mqtt_client.hpp"

#include <cassert>
#include <iostream>

MqttClient::MqttClient(const std::string& client_id) {
    mosquitto_lib_init();
    mosq_ = mosquitto_new(client_id.c_str(), true, this);
    assert(mosq_);

    mosquitto_message_callback_set(mosq_, on_message_static);
}

MqttClient::~MqttClient() {
    if (mosq_) {
        mosquitto_destroy(mosq_);
    }
    mosquitto_lib_cleanup();
}

bool MqttClient::connect(const std::string &host, int port, int keepalive) {
    return mosquitto_connect(mosq_, host.c_str(), port, keepalive);
}

bool MqttClient::publish(const std::string &topic, const std::string &payload, int qos) {
    return mosquitto_publish(mosq_, nullptr, topic.c_str(), payload.size(), payload.c_str(), qos, false);
}

bool MqttClient::subscribe(const std::string& topic, int qos) {
    return mosquitto_subscribe(mosq_, nullptr, topic.c_str(), qos);
}

void MqttClient::loop_start() {
    mosquitto_loop_start(mosq_);
}

void MqttClient::loop_stop() {
    mosquitto_loop_stop(mosq_, true);
}

void MqttClient::set_message_handler(MessageHandler handler) {
    handler_ = std::move(handler);
}

void MqttClient::on_message_static(struct mosquitto*, void* obj, const struct mosquitto_message* msg) {
    static_cast<MqttClient*>(obj)->on_message(msg);
}

void MqttClient::on_message(const struct mosquitto_message* msg) {
    if (handler_) {
        std::string topic = msg->topic;
        std::string payload(static_cast<char*>(msg->payload), msg->payloadlen);
        handler_(topic, payload);
    }
}