// Copyright 2026 Kevin Landers <kmlanders@gmail.com>

#pragma once

#include "telemetry/i_mqtt_client.hpp"

#include <boost/asio.hpp>
#include <async_mqtt/all.hpp>

#include <atomic>
#include <memory>
#include <string>
#include <thread>

namespace asio = boost::asio;
namespace am   = async_mqtt;

class AsyncMqttClient : public IMqttClient {
public:
    explicit AsyncMqttClient(const std::string& client_id);
    ~AsyncMqttClient() override;

    bool connect(const std::string& host, int port, int keepalive = 60) override;
    bool publish(const std::string& topic, const std::string& payload, int qos = 1) override;
    bool subscribe(const std::string& topic, int qos = 1) override;
    bool loop_start() override;
    bool loop_stop() override;
    void set_message_handler(MessageHandler handler) override;
    void set_connect_handler(ConnectHandler handler) override;
    void set_disconnect_handler(DisconnectHandler handler) override;

private:
    // Named coroutine methods avoid GCC 11 ICE with nested lambda awaitables.
    asio::awaitable<void> co_connect(std::string host, std::string port_str,
                                      std::uint16_t keepalive,
                                      std::shared_ptr<std::promise<bool>> prom);
    asio::awaitable<void> co_publish(std::string topic, std::string payload, int qos);
    asio::awaitable<void> co_subscribe(std::string topic, int qos);
    asio::awaitable<void> co_disconnect();
    asio::awaitable<void> recv_loop();

    using AmClient = am::client<am::protocol_version::v3_1_1, am::protocol::mqtt>;

    std::string client_id_;

    // ioc_ must be declared before work_guard_ and amcl_ (init order matters)
    asio::io_context ioc_;
    asio::executor_work_guard<asio::io_context::executor_type> work_guard_;
    AmClient amcl_;

    std::thread       ioc_thread_;
    std::atomic<bool> running_{false};

    MessageHandler    message_handler_;
    ConnectHandler    connect_handler_;
    DisconnectHandler disconnect_handler_;
};
