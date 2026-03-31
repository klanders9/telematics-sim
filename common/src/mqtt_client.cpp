// Copyright 2026 Kevin Landers <kmlanders@gmail.com>

#include "telemetry/mqtt_client.hpp"

#include <iostream>
#include <mutex>
#include <stdexcept>

MqttClient::MqttClient(const std::string& client_id) {
    // mosquitto_lib_init/cleanup are global operations; call them exactly once
    // per process regardless of how many MqttClient instances are constructed.
    static std::once_flag lib_init_flag;
    std::call_once(lib_init_flag, [] { mosquitto_lib_init(); });

    mosq_ = mosquitto_new(client_id.c_str(), true, this);
    if (!mosq_) {
        throw std::runtime_error("mosquitto_new failed: out of memory or invalid client_id");
    }

    mosquitto_message_callback_set(mosq_, on_message_static);
    mosquitto_connect_callback_set(mosq_, on_connect_static);
    mosquitto_disconnect_callback_set(mosq_, on_disconnect_static);
}

MqttClient::~MqttClient() {
    if (mosq_) {
        mosquitto_destroy(mosq_);
    }
    // lib cleanup is intentionally omitted here: with call_once init, cleanup at
    // process exit is acceptable and avoids double-cleanup if multiple instances
    // existed. Call mosquitto_lib_cleanup() from main() if explicit cleanup matters.
}

bool MqttClient::connect(const std::string& host, int port, int keepalive) {
    auto rc = mosquitto_connect(mosq_, host.c_str(), port, keepalive);
    if (rc != MOSQ_ERR_SUCCESS) {
        std::cerr << "MQTT connect error: " << mosquitto_strerror(rc) << std::endl;
        return false;
    }
    return true;
}

bool MqttClient::publish(const std::string& topic, const std::string& payload, int qos) {
    auto rc = mosquitto_publish(mosq_, nullptr, topic.c_str(),
                                static_cast<int>(payload.size()), payload.data(), qos, false);
    if (rc != MOSQ_ERR_SUCCESS) {
        std::cerr << "Publish error: " << mosquitto_strerror(rc) << std::endl;
        return false;
    }
    return true;
}

bool MqttClient::subscribe(const std::string& topic, int qos) {
    auto rc = mosquitto_subscribe(mosq_, nullptr, topic.c_str(), qos);
    if (rc != MOSQ_ERR_SUCCESS) {
        std::cerr << "Subscribe error: " << mosquitto_strerror(rc) << std::endl;
        return false;
    }
    return true;
}

bool MqttClient::loop_start() {
    auto rc = mosquitto_loop_start(mosq_);
    if (rc != MOSQ_ERR_SUCCESS) {
        std::cerr << "loop_start error: " << mosquitto_strerror(rc) << std::endl;
        return false;
    }
    return true;
}

bool MqttClient::loop_stop() {
    auto rc = mosquitto_loop_stop(mosq_, true);
    if (rc != MOSQ_ERR_SUCCESS) {
        std::cerr << "loop_stop error: " << mosquitto_strerror(rc) << std::endl;
        return false;
    }
    return true;
}

void MqttClient::set_message_handler(MessageHandler handler) {
    handler_ = std::move(handler);
}

void MqttClient::set_connect_handler(ConnectHandler handler) {
    connect_handler_ = std::move(handler);
}

void MqttClient::set_disconnect_handler(DisconnectHandler handler) {
    disconnect_handler_ = std::move(handler);
}

// --- static trampoline callbacks ---

void MqttClient::on_message_static(struct mosquitto*, void* obj, const struct mosquitto_message* msg) {
    static_cast<MqttClient*>(obj)->on_message(msg);
}

void MqttClient::on_connect_static(struct mosquitto*, void* obj, int rc) {
    static_cast<MqttClient*>(obj)->on_connect(rc);
}

void MqttClient::on_disconnect_static(struct mosquitto*, void* obj, int rc) {
    static_cast<MqttClient*>(obj)->on_disconnect(rc);
}

// --- instance callbacks ---

void MqttClient::on_message(const struct mosquitto_message* msg) {
    if (handler_) {
        std::string topic = msg->topic;
        std::string payload(static_cast<char*>(msg->payload), msg->payloadlen);
        handler_(topic, payload);
    }
}

void MqttClient::on_connect(int rc) {
    if (rc != 0) {
        std::cerr << "MQTT broker rejected connection, rc=" << rc << std::endl;
    }
    if (connect_handler_) {
        connect_handler_(rc);
    }
}

void MqttClient::on_disconnect(int rc) {
    if (rc != 0) {
        // rc != 0 means unexpected disconnect; mosquitto will auto-reconnect
        std::cerr << "MQTT unexpected disconnect, rc=" << rc << std::endl;
    }
    if (disconnect_handler_) {
        disconnect_handler_(rc);
    }
}
