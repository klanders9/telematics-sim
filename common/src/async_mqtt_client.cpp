// Copyright 2026 Kevin Landers <kmlanders@gmail.com>

#include "telemetry/async_mqtt_client.hpp"

#include <future>
#include <iostream>
#include <vector>

// ---------------------------------------------------------------------------
// Construction / destruction
// ---------------------------------------------------------------------------

AsyncMqttClient::AsyncMqttClient(const std::string& client_id)
    : client_id_(client_id),
      ioc_(),
      work_guard_(asio::make_work_guard(ioc_)),
      amcl_(ioc_.get_executor())
{}

AsyncMqttClient::~AsyncMqttClient() {
    if (ioc_thread_.joinable()) {
        running_ = false;
        work_guard_.reset();
        ioc_.stop();
        ioc_thread_.join();
    }
}

// ---------------------------------------------------------------------------
// Named coroutines
// (Named methods avoid GCC 11 ICE with nested lambda awaitables.
//  Packets are pre-constructed before co_await to avoid further ICE triggers.)
// ---------------------------------------------------------------------------

asio::awaitable<void> AsyncMqttClient::co_connect(
        std::string host, std::string port_str,
        std::uint16_t keepalive,
        std::shared_ptr<std::promise<bool>> prom) {

    try {
        co_await amcl_.async_underlying_handshake(host, port_str, asio::use_awaitable);

        am::v3_1_1::connect_packet pkt{true, keepalive, client_id_,
                                        std::nullopt, std::nullopt, std::nullopt};
        auto connack_opt = co_await amcl_.async_start(std::move(pkt), asio::use_awaitable);

        if (!connack_opt) {
            std::cerr << "[AsyncMqttClient] No CONNACK received" << std::endl;
            prom->set_value(false);
            co_return;
        }

        if (connack_opt->code() != am::connect_return_code::accepted) {
            std::cerr << "[AsyncMqttClient] Connection refused, code="
                      << static_cast<int>(connack_opt->code()) << std::endl;
            prom->set_value(false);
            co_return;
        }

        if (connect_handler_) connect_handler_(0);
        prom->set_value(true);

    } catch (const boost::system::system_error& e) {
        std::cerr << "[AsyncMqttClient] connect error: " << e.what() << std::endl;
        prom->set_value(false);
    }
}

asio::awaitable<void> AsyncMqttClient::co_publish(
        std::string topic, std::string payload, int qos) {

    const auto q = static_cast<am::qos>(qos & 0x3);
    if (q == am::qos::at_most_once) {
        am::v3_1_1::publish_packet pkt{topic, payload, am::qos::at_most_once};
        co_await amcl_.async_publish(std::move(pkt), asio::use_awaitable);
    } else {
        auto pid_opt = amcl_.acquire_unique_packet_id();
        if (!pid_opt) {
            std::cerr << "[AsyncMqttClient] packet id exhausted" << std::endl;
            co_return;
        }
        am::v3_1_1::publish_packet pkt{*pid_opt, topic, payload, q};
        co_await amcl_.async_publish(std::move(pkt), asio::use_awaitable);
    }
}

asio::awaitable<void> AsyncMqttClient::co_subscribe(std::string topic, int qos) {
    auto pid_opt = amcl_.acquire_unique_packet_id();
    if (!pid_opt) {
        std::cerr << "[AsyncMqttClient] subscribe: no packet id" << std::endl;
        co_return;
    }
    std::vector<am::topic_subopts> sub_entries;
    sub_entries.emplace_back(am::topic_subopts{topic, static_cast<am::qos>(qos & 0x3)});
    am::v3_1_1::subscribe_packet pkt{*pid_opt, am::force_move(sub_entries)};
    co_await amcl_.async_subscribe(std::move(pkt), asio::use_awaitable);
}

asio::awaitable<void> AsyncMqttClient::co_disconnect() {
    am::v3_1_1::disconnect_packet pkt{};
    co_await amcl_.async_disconnect(std::move(pkt), asio::use_awaitable);
    work_guard_.reset();
}

asio::awaitable<void> AsyncMqttClient::recv_loop() {
    while (running_) {
        auto pv_opt = co_await amcl_.async_recv(asio::use_awaitable);
        if (!pv_opt) break;

        if (auto* p = pv_opt->template get_if<AmClient::publish_packet>()) {
            if (message_handler_) {
                message_handler_(std::string(p->topic()), std::string(p->payload()));
            }
        } else if (pv_opt->template get_if<AmClient::disconnect_packet>()) {
            running_ = false;
            if (disconnect_handler_) disconnect_handler_(1);
        }
    }
}

// ---------------------------------------------------------------------------
// IMqttClient implementation
// ---------------------------------------------------------------------------

bool AsyncMqttClient::connect(const std::string& host, int port, int keepalive) {
    if (!ioc_thread_.joinable()) {
        ioc_thread_ = std::thread([this] { ioc_.run(); });
    }

    auto prom = std::make_shared<std::promise<bool>>();
    auto fut  = prom->get_future();

    asio::co_spawn(ioc_,
        co_connect(host, std::to_string(port),
                   static_cast<std::uint16_t>(keepalive), prom),
        asio::detached);

    if (fut.wait_for(std::chrono::seconds(10)) != std::future_status::ready) {
        std::cerr << "[AsyncMqttClient] connect timed out" << std::endl;
        return false;
    }
    return fut.get();
}

bool AsyncMqttClient::publish(const std::string& topic, const std::string& payload, int qos) {
    asio::co_spawn(ioc_, co_publish(topic, payload, qos), asio::detached);
    return true;
}

bool AsyncMqttClient::subscribe(const std::string& topic, int qos) {
    asio::co_spawn(ioc_, co_subscribe(topic, qos), asio::detached);
    return true;
}

bool AsyncMqttClient::loop_start() {
    running_ = true;
    asio::co_spawn(ioc_, recv_loop(), asio::detached);
    return true;
}

bool AsyncMqttClient::loop_stop() {
    running_ = false;
    asio::co_spawn(ioc_, co_disconnect(), asio::detached);
    if (ioc_thread_.joinable()) ioc_thread_.join();
    return true;
}

void AsyncMqttClient::set_message_handler(MessageHandler handler) {
    message_handler_ = std::move(handler);
}

void AsyncMqttClient::set_connect_handler(ConnectHandler handler) {
    connect_handler_ = std::move(handler);
}

void AsyncMqttClient::set_disconnect_handler(DisconnectHandler handler) {
    disconnect_handler_ = std::move(handler);
}
