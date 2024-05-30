#include "RabbitMQ.h"

#include <zlog.h>

#include <chrono>
#include <cstring>
#include <iostream>

using namespace std;
using namespace Rabbit;

RabbitMQClient::RabbitMQClient(const string &host, uint16_t port, const string &user, const string &password)
    : host_(host)
    , port_(port)
    , user_(user)
    , password_(password)
    , channel_id_(0)
    , socket_(nullptr)
    , conn_(nullptr) {
    dzlog_debug("RabbitMQClient::RabbitMQClient()");
}

RabbitMQClient::~RabbitMQClient() {
    if (channel_id_ != 0) {
        amqp_channel_close(conn_, channel_id_, AMQP_REPLY_SUCCESS);
    }
    if (socket_ != nullptr) {
        amqp_connection_close(conn_, AMQP_REPLY_SUCCESS);
    }
    if (conn_ != nullptr) {
        amqp_destroy_connection(conn_);
    }
    dzlog_debug("RabbitMQClient::~RabbitMQClient()");
}

int RabbitMQClient::Connect(const string &vhost) {
    conn_ = amqp_new_connection(); // new connection ojbect
    if (conn_ == nullptr) {
        dzlog_error("new connection failed");
        return errMessage(AMQP_STATUS_NO_MEMORY);
    }
    socket_ = amqp_tcp_socket_new(conn_); // bind connection object to socket
    if (socket_ == nullptr) {
        dzlog_error("new socket failed");
        return errMessage(AMQP_STATUS_NO_MEMORY);
    }
    // set socket options if necessary
    auto status = amqp_socket_open(socket_, host_.c_str(), port_); // open socket
    if (status != AMQP_STATUS_OK) {
        dzlog_error("socket open failed");
        return errMessage((amqp_status_enum)status);
    }
    auto reply = amqp_login(conn_, vhost.c_str(), 0, AMQP_DEFAULT_FRAME_SIZE, 0, AMQP_SASL_METHOD_PLAIN, user_.c_str(),
                            password_.c_str()); // login rabbit mq server
    if (reply.reply_type != AMQP_RESPONSE_NORMAL) {
        dzlog_error("login failed");
        return errMessage(reply);
    }
    const int single_channel_id = 1;
    amqp_channel_open(conn_, single_channel_id);
    reply = amqp_get_rpc_reply(conn_);
    if (reply.reply_type != AMQP_RESPONSE_NORMAL) {
        dzlog_error("channel open failed");
        return errMessage(reply);
    }
    // which means the client is ready to use
    channel_id_ = single_channel_id; // one client with one connection with one channel
    dzlog_debug("RabbitMQClient::Connect() success");
    return 0;
}

int RabbitMQClient::ExchangeDeclare(const string &exchange, const string &type, bool passive, bool durable,
                                    bool auto_delete, bool internal) {
    if (channel_id_ == 0) {
        dzlog_error("channel id is 0");
        return -1;
    }
    auto exchange_name = amqp_cstring_bytes(exchange.c_str());
    auto type_name = amqp_cstring_bytes(type.c_str());
    amqp_exchange_declare(conn_, channel_id_, exchange_name, type_name, passive, durable, auto_delete, internal,
                          amqp_empty_table);
    auto reply = amqp_get_rpc_reply(conn_);
    if (reply.reply_type != AMQP_RESPONSE_NORMAL) {
        dzlog_error("exchange declare failed");
        return errMessage(reply);
    }
    dzlog_debug("RabbitMQClient::ExchangeDeclare() success");
    return 0;
}

int RabbitMQClient::QueueDeclare(const string &queue, bool passive, bool durable, bool auto_delete, bool internal) {
    if (channel_id_ == 0) {
        dzlog_error("channel id is 0");
        return -1;
    }
    auto queue_name = amqp_cstring_bytes(queue.c_str());
    amqp_queue_declare(conn_, channel_id_, queue_name, passive, durable, auto_delete, internal, amqp_empty_table);
    auto reply = amqp_get_rpc_reply(conn_);
    if (reply.reply_type != AMQP_RESPONSE_NORMAL) {
        dzlog_error("queue declare failed");
        return errMessage(reply);
    }
    dzlog_debug("RabbitMQClient::QueueDeclare() success");
    return 0;
}

int RabbitMQClient::QueueBind(const string &queue, const string &exchange, const string &binding_key) {
    if (channel_id_ == 0) {
        dzlog_error("channel id is 0");
        return -1;
    }
    auto queue_name = amqp_cstring_bytes(queue.c_str());
    auto exchange_name = amqp_cstring_bytes(exchange.c_str());
    auto binding_key_name = amqp_cstring_bytes(binding_key.c_str());
    amqp_queue_bind(conn_, channel_id_, queue_name, exchange_name, binding_key_name, amqp_empty_table);
    auto reply = amqp_get_rpc_reply(conn_);
    if (reply.reply_type != AMQP_RESPONSE_NORMAL) {
        dzlog_error("queue bind failed");
        return errMessage(reply);
    }
    dzlog_debug("RabbitMQClient::QueueBind() success");
    return 0;
}

int RabbitMQClient::QueueUnbind(const string &queue, const string &exchange, const string &binding_key) {
    if (channel_id_ == 0) {
        dzlog_error("channel id is 0");
        return -1;
    }
    auto queue_name = amqp_cstring_bytes(queue.c_str());
    auto exchange_name = amqp_cstring_bytes(exchange.c_str());
    auto binding_key_name = amqp_cstring_bytes(binding_key.c_str());
    amqp_queue_unbind(conn_, channel_id_, queue_name, exchange_name, binding_key_name, amqp_empty_table);
    auto reply = amqp_get_rpc_reply(conn_);
    if (reply.reply_type != AMQP_RESPONSE_NORMAL) {
        dzlog_error("queue unbind failed");
        return errMessage(reply);
    }
    dzlog_debug("RabbitMQClient::QueueUnbind() success");
    return 0;
}

int RabbitMQClient::QueueDelete(const string &queue, bool if_unused, bool if_empty) {
    if (channel_id_ == 0) {
        dzlog_error("channel id is 0");
        return -1;
    }
    auto queue_name = amqp_cstring_bytes(queue.c_str());
    amqp_queue_delete(conn_, channel_id_, queue_name, if_unused, if_empty);
    auto reply = amqp_get_rpc_reply(conn_);
    if (reply.reply_type != AMQP_RESPONSE_NORMAL) {
        dzlog_error("queue delete failed");
        return errMessage(reply);
    }
    dzlog_debug("RabbitMQClient::QueueDelete() success");
    return 0;
}

int RabbitMQClient::errMessage(amqp_status_enum status) {
    switch (status) {
    case AMQP_STATUS_OK:
        dzlog_debug("operation completed successfully");
        break;
    default:
        dzlog_error("error: %s", amqp_error_string2(status));
    }
    return status == AMQP_STATUS_OK ? 0 : -1;
}

int RabbitMQClient::errMessage(amqp_rpc_reply_t_ reply) {
    switch (reply.reply_type) {
    case AMQP_RESPONSE_NORMAL:
        dzlog_debug("rpc completed successfully");
        break;
    case AMQP_RESPONSE_NONE:
        dzlog_error("error: EOF form the socket");
        break;
    case AMQP_RESPONSE_LIBRARY_EXCEPTION:
        dzlog_error("error: library exception %s", amqp_error_string2(reply.library_error));
        break;
    case AMQP_RESPONSE_SERVER_EXCEPTION:
        switch (reply.reply.id) {
        case AMQP_CHANNEL_CLOSE_METHOD:
            dzlog_error("error: server channel close");
            break;
        case AMQP_CONNECTION_CLOSE_METHOD:
            dzlog_error("error: server connection close");
            break;
        default:
            dzlog_error("error: unknown server error, method id: %d", reply.reply.id);
        }
        break;
    default:
        dzlog_error("error: unknown error");
    }
    return reply.reply_type == AMQP_RESPONSE_NORMAL ? 0 : -1;
}

RabbitMQPublisher::RabbitMQPublisher(const string &host, uint16_t port, const string &user, const string &password)
    : RabbitMQClient(host, port, user, password) {
    dzlog_debug("RabbitMQPublisher::RabbitMQPublisher()");
}

RabbitMQPublisher::~RabbitMQPublisher() {
    dzlog_debug("RabbitMQPublisher::~RabbitMQPublisher()");
}

int RabbitMQPublisher::Publish(const string &exchange, const string &routing_key, const string &message) {
    if (channel_id_ == 0) {
        dzlog_error("channel id is 0");
        return -1;
    }
    auto exchange_name = amqp_cstring_bytes(exchange.c_str());
    auto routing_key_name = amqp_cstring_bytes(routing_key.c_str());

    amqp_basic_publish(conn_, channel_id_, exchange_name, routing_key_name, 0, 0, nullptr,
                       amqp_cstring_bytes(message.c_str()));
    auto reply = amqp_get_rpc_reply(conn_);
    if (reply.reply_type != AMQP_RESPONSE_NORMAL) {
        dzlog_error("publish failed");
        return errMessage(reply);
    }
    return 0;
}

RabbitMQConsumer::RabbitMQConsumer(const string &host, uint16_t port, const string &user, const string &password)
    : RabbitMQClient(host, port, user, password) {
    dzlog_debug("RabbitMQConsumer::RabbitMQConsumer()");
}

RabbitMQConsumer::~RabbitMQConsumer() {
    dzlog_debug("RabbitMQConsumer::~RabbitMQConsumer()");
}

int RabbitMQConsumer::Prepare(const string &queue) {
    if (channel_id_ == 0) {
        dzlog_error("channel id is 0");
        return -1;
    }

    amqp_basic_qos(conn_, channel_id_, 0, 1, 0);
    auto queue_name = amqp_cstring_bytes(queue.c_str());
    amqp_basic_consume(conn_, channel_id_, queue_name, amqp_empty_bytes, 0, 0, 0, amqp_empty_table);
    auto reply = amqp_get_rpc_reply(conn_);
    if (reply.reply_type != AMQP_RESPONSE_NORMAL) {
        dzlog_error("amqp_basic_consume failed");
        return errMessage(reply);
    }
    dzlog_debug("RabbitMQConsumer::Prepare() success");
    return 0;
}

int RabbitMQConsumer::Consume(string &message, timeval timeout) {
    if (channel_id_ == 0) {
        dzlog_error("channel id is 0");
        return -1;
    }

    message.clear();
    if (timeout.tv_sec == 0 && timeout.tv_usec == 0) {
        timeout.tv_sec = 2; // 2 seconds
    }

    amqp_maybe_release_buffers(conn_);
    amqp_envelope_t envelope;
    auto result = amqp_consume_message(conn_, &envelope, &timeout, 0);
    if (result.reply_type != AMQP_RESPONSE_NORMAL) {
        if (result.reply_type == AMQP_RESPONSE_LIBRARY_EXCEPTION && result.library_error == AMQP_STATUS_TIMEOUT) {
            dzlog_debug("amqp_consume_message timeout");
            return 0;
        }
        dzlog_error("amqp_consume_message failed %d %d", result.reply_type, result.library_error);
        errMessage(result);
        return -1;
    }

    message = string((char *)envelope.message.body.bytes, envelope.message.body.len);
    if (amqp_basic_ack(conn_, channel_id_, envelope.delivery_tag, 0) != 0) {
        dzlog_warn("amqp_basic_ack failed %lu %s", envelope.delivery_tag, message.c_str());
    } else {
        // dzlog_debug("amqp_basic_ack success %lu %s", envelope.delivery_tag, message.c_str());
    }
    amqp_destroy_envelope(&envelope);
    return 0;
}
