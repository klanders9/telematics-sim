// Copyright 2026 Kevin Landers <kmlanders@gmail.com>

#include "telemetry/mosquitto_client.hpp"

#include <iostream>
#include <mutex>
#include <stdexcept>

MosquittoClient::MosquittoClient(const std::string& client_id) {
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

MosquittoClient::~MosquittoClient() {
    if (mosq_) {
        mosquitto_destroy(mosq_);
    }
}

bool MosquittoClient::connect(const std::string& host, int port, int keepalive) {
    auto rc = mosquitto_connect(mosq_, host.c_str(), port, keepalive);
    if (rc != MOSQ_ERR_SUCCESS) {
        std::cerr << "MQTT connect error: " << mosquitto_strerror(rc) << std::endl;
        return false;
    }
    return true;
}

bool MosquittoClient::publish(const std::string& topic, const std::string& payload, int qos) {
    auto rc = mosquitto_publish(mosq_, nullptr, topic.c_str(),
                                static_cast<int>(payload.size()), payload.data(), qos, false);
    if (rc != MOSQ_ERR_SUCCESS) {
        std::cerr << "Publish error: " << mosquitto_strerror(rc) << std::endl;
        return false;
    }
    return true;
}

bool MosquittoClient::subscribe(const std::string& topic, int qos) {
    auto rc = mosquitto_subscribe(mosq_, nullptr, topic.c_str(), qos);
    if (rc != MOSQ_ERR_SUCCESS) {
        std::cerr << "Subscribe error: " << mosquitto_strerror(rc) << std::endl;
        return false;
    }
    return true;
}

bool MosquittoClient::loop_start() {
    auto rc = mosquitto_loop_start(mosq_);
    if (rc != MOSQ_ERR_SUCCESS) {
        std::cerr << "loop_start error: " << mosquitto_strerror(rc) << std::endl;
        return false;
    }
    return true;
}

bool MosquittoClient::loop_stop() {
    auto rc = mosquitto_loop_stop(mosq_, true);
    if (rc != MOSQ_ERR_SUCCESS) {
        std::cerr << "loop_stop error: " << mosquitto_strerror(rc) << std::endl;
        return false;
    }
    return true;
}

void MosquittoClient::set_message_handler(MessageHandler handler) {
    message_handler_ = std::move(handler);
}

void MosquittoClient::set_connect_handler(ConnectHandler handler) {
    connect_handler_ = std::move(handler);
}

void MosquittoClient::set_disconnect_handler(DisconnectHandler handler) {
    disconnect_handler_ = std::move(handler);
}

void MosquittoClient::on_message_static(struct mosquitto*, void* obj,
                                         const struct mosquitto_message* msg) {
    static_cast<MosquittoClient*>(obj)->on_message(msg);
}

void MosquittoClient::on_connect_static(struct mosquitto*, void* obj, int rc) {
    static_cast<MosquittoClient*>(obj)->on_connect(rc);
}

void MosquittoClient::on_disconnect_static(struct mosquitto*, void* obj, int rc) {
    static_cast<MosquittoClient*>(obj)->on_disconnect(rc);
}

void MosquittoClient::on_message(const struct mosquitto_message* msg) {
    if (message_handler_) {
        std::string topic = msg->topic;
        std::string payload(static_cast<char*>(msg->payload), msg->payloadlen);
        message_handler_(topic, payload);
    }
}

void MosquittoClient::on_connect(int rc) {
    if (rc != 0) {
        std::cerr << "MQTT broker rejected connection, rc=" << rc << std::endl;
    }
    if (connect_handler_) {
        connect_handler_(rc);
    }
}

void MosquittoClient::on_disconnect(int rc) {
    if (rc != 0) {
        std::cerr << "MQTT unexpected disconnect, rc=" << rc << std::endl;
    }
    if (disconnect_handler_) {
        disconnect_handler_(rc);
    }
}
