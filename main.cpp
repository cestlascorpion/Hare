#include <zlog.h>

#include <future>
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
        auto ret = helper->Connect();
        if (ret != 0) {
            dzlog_debug("Connect failed: %d", ret);
            return -1;
        }
        dzlog_debug("re-Connect success");
        ret = helper->QueueDelete(queue, false, false);
        if (ret != 0) {
            dzlog_debug("QueueDelete failed: %d", ret);
            return -1;
        }
        dzlog_debug("QueueDelete success");
    }
    vector<future<void>> pubFutures;
    for (int id = 0; id < 5; ++id) {
        pubFutures.push_back(async(
            launch::async,
            [](int i) {
                auto publisher = GetPublisher();
                if (!publisher) {
                    dzlog_error("GetPublisher %d failed", i);
                    return;
                }
                for (int j = 0; j < 200; ++j) {
                    auto message = generate_message(i, j);
                    if (publisher->Publish("exchange", "routing_key", message) != 0) {
                        dzlog_error("Publish %d failed", i);
                        return;
                    }
                    // dzlog_debug("Publish success: %s", message.c_str());
                    this_thread::sleep_for(chrono::milliseconds(100));
                }
                dzlog_info("Publish %d finished", i);
            },
            id));
    }
    for (auto& f : pubFutures) {
        f.wait();
    }
    auto consumer = GetConsumer();
    if (!consumer) {
        dzlog_error("GetConsumer failed");
        return -1;
    }
    if (consumer->Prepare(queue) != 0) {
        dzlog_error("Prepare failed");
        return -1;
    }
    auto cnt = 0;
    string message;
    while (true) {
        if (consumer == nullptr) {
            consumer = GetConsumer();
            if (consumer == nullptr) {
                dzlog_error("re-connect failed");
                this_thread::sleep_for(chrono::seconds(5));
                continue;
            }
        }
        auto ret = consumer->Consume(message, {2, 0});
        if (ret != 0) {
            dzlog_error("Consume failed: %d", ret);
            consumer.reset();
            this_thread::sleep_for(chrono::seconds(5));
        }
        if (message.empty()) {
            dzlog_debug("Consume timeout");
            continue;
        }
        // dzlog_info("Consume success: %s", message.c_str());
        dzlog_info("Consume success");
        this_thread::sleep_for(chrono::milliseconds(200));
        if (++cnt >= 1000) {
            dzlog_info("Consume finished");
            break;
        }
    }
    return 0;
}