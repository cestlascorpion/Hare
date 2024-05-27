#include <zlog.h>

#include <cassert>
#include <memory>
#include <string>
#include <vector>

#include "RabbitMQ.h"

using namespace std;
using namespace Rabbit;

const string host = "127.0.0.1";
const uint16_t port = 5672;
const string user = "guest";
const string password = "guest";
const string vhost = "/";
const string exchange = "exchange";
const string type = "direct";
const string queue = "queue";
const string routing_key = "routing_key";

string generate_message(int id, int idx) {
    char buffer[1024]{0};
    snprintf(buffer, sizeof(buffer),
             "[%02d]message-%02d 1234567890-=qwertyuiop[]asdfghjkl;'zxcvbnm,. 如果有中文会怎么样？", id, idx);
    return string(buffer);
}

unique_ptr<RabbitMQClient> GetHelper() {
    auto client = make_unique<RabbitMQClient>(host, port, user, password);

    assert(client->ExchangeDeclare("exchange", type, false, true, false, false) != 0);
    assert(client->QueueDeclare(queue, false, true, false, false) != 0);
    assert(client->QueueBind(queue, "exchange", routing_key) != 0);

    if (client->Connect(vhost) != 0) {
        dzlog_debug("connect failed");
        return nullptr;
    }
    if (client->ExchangeDeclare("exchange", type, false, true, false, false) != 0) {
        dzlog_debug("ExchangeDeclare failed");
        return nullptr;
    }
    if (client->QueueDeclare(queue, false, true, false, false) != 0) {
        dzlog_debug("QueueDeclare failed");
        return nullptr;
    }
    if (client->QueueBind(queue, "exchange", routing_key) != 0) {
        dzlog_debug("QueueBind failed");
        return nullptr;
    }
    return client;
}

unique_ptr<RabbitMQPublisher> GetPublisher() {
    auto publisher = make_unique<RabbitMQPublisher>(host, port, user, password);

    assert(publisher->Publish("exchange", "routing_key", "hello world") != 0);

    if (publisher->Connect(vhost) != 0) {
        dzlog_debug("connect failed");
        return nullptr;
    }
    if (publisher->ExchangeDeclare("exchange", type, false, true, false, false) != 0) {
        dzlog_debug("ExchangeDeclare failed");
        return nullptr;
    }
    if (publisher->QueueDeclare(queue, false, true, false, false) != 0) {
        dzlog_debug("QueueDeclare failed");
        return nullptr;
    }
    if (publisher->QueueBind(queue, "exchange", routing_key) != 0) {
        dzlog_debug("QueueBind failed");
        return nullptr;
    }
    return publisher;
}

unique_ptr<RabbitMQConsumer> GetConsumer() {
    auto consumer = make_unique<RabbitMQConsumer>(host, port, user, password);

    assert(consumer->Consume(queue, [](const string &) {}) != 0);

    if (consumer->Connect(vhost) != 0) {
        dzlog_debug("connect failed");
        return nullptr;
    }
    if (consumer->ExchangeDeclare("exchange", type, false, true, false, false) != 0) {
        dzlog_debug("ExchangeDeclare failed");
        return nullptr;
    }
    if (consumer->QueueDeclare(queue, false, true, false, false) != 0) {
        dzlog_debug("QueueDeclare failed");
        return nullptr;
    }
    if (consumer->QueueBind(queue, "exchange", routing_key) != 0) {
        dzlog_debug("QueueBind failed");
        return nullptr;
    }
    return consumer;
}

int main() {
    if (dzlog_init("../zlog.conf", "default") != 0) {
        printf("zlog init failed\n");
        return -1;
    }
    {
        auto helper = GetHelper();
        if (!helper) {
            dzlog_debug("GetHelper failed");
            return -1;
        }
        auto ret = helper->QueueDelete(queue, false, false);
        if (ret != 0) {
            dzlog_debug("QueueDelete failed: %d", ret);
            return -1;
        }
        dzlog_debug("QueueDelete success");
    }

    auto publisher = GetPublisher();
    if (!publisher) {
        dzlog_debug("GetPublisher failed");
        return -1;
    }

    vector<unique_ptr<RabbitMQConsumer>> consumers;
    vector<future<void>> pubFutures;
    for (int i = 0; i < 5; ++i) {
        auto consumer = GetConsumer();
        if (!consumer) {
            dzlog_debug("GetConsumer failed");
            return -1;
        }
        consumer->Consume(queue, [id = i](const string &message) {
            // do nothing
            dzlog_info("[%02d]Consume: %s", id, message.c_str());
        });
        assert(consumer->Consume(queue, [id = i](const string &message) {
            // do nothing
            dzlog_info("[%02d]Consume: %s", id, message.c_str());
        }) != 0);
        consumers.push_back(move(consumer));

        auto fut = std::async([pub = publisher.get(), i]() {
            for (int j = 0; j < 20; ++j) {
                auto message = generate_message(i, j);
                if (pub->Publish("exchange", "routing_key", message) != 0) {
                    dzlog_debug("Publish failed");
                    break;
                }
                dzlog_info("Publish success: %s", message.c_str());
                this_thread::sleep_for(chrono::milliseconds(100));
            }
        });
        pubFutures.push_back(move(fut));
    }
    for (auto &fut : pubFutures) {
        fut.wait();
    }
    this_thread::sleep_for(chrono::seconds(30)); // I don't know when the consume is finished, so do you
    return 0;
}