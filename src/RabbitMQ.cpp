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
        auto reply = amqp_channel_close(conn_, channel_id_, AMQP_REPLY_SUCCESS);
        if (reply.reply_type != AMQP_RESPONSE_NORMAL) {
            dzlog_error("channel close failed");
            errMessage(reply);
        }
    }
    if (socket_ != nullptr) {
        auto reply = amqp_connection_close(conn_, AMQP_REPLY_SUCCESS);
        if (reply.reply_type != AMQP_RESPONSE_NORMAL) {
            dzlog_error("connection close failed");
            errMessage(reply);
        }
    }
    if (conn_ != nullptr) {
        auto status = amqp_destroy_connection(conn_);
        if (status != AMQP_STATUS_OK) {
            dzlog_error("destroy connection failed");
            errMessage((amqp_status_enum)status);
        }
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
    auto reply = amqp_login(conn_, vhost.c_str(), 1, AMQP_DEFAULT_FRAME_SIZE, 0, AMQP_SASL_METHOD_PLAIN, user_.c_str(),
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

    std::lock_guard<std::mutex> lock(mutex_); // amqp_basic_publish/amqp_get_rpc_reply are not thread-safe(I suppose)
    amqp_basic_publish(conn_, channel_id_, exchange_name, routing_key_name, 0, 0, nullptr,
                       amqp_cstring_bytes(message.c_str()));
    auto reply = amqp_get_rpc_reply(conn_);
    if (reply.reply_type != AMQP_RESPONSE_NORMAL) {
        dzlog_error("publish failed");
        return errMessage(reply);
    }
    dzlog_debug("RabbitMQPublisher::Publish() success");
    return 0;
}

RabbitMQConsumer::RabbitMQConsumer(const string &host, uint16_t port, const string &user, const string &password)
    : RabbitMQClient(host, port, user, password) {
    dzlog_debug("RabbitMQConsumer::RabbitMQConsumer()");
}

RabbitMQConsumer::~RabbitMQConsumer() {
    if (thread_.joinable()) {
        promise_.set_value();
        thread_.join();
    }
    dzlog_debug("RabbitMQConsumer::~RabbitMQConsumer()");
}

int RabbitMQConsumer::Consume(const string &queue, function<void(const string &)> callback) {
    if (channel_id_ == 0) {
        dzlog_error("channel id is 0");
        return -1;
    }
    std::lock_guard<std::mutex> lock(mutex_); // amqp_basic_consume/amqp_get_rpc_reply are not thread-safe(I suppose)
    if (thread_.joinable()) {
        dzlog_error("consume thread is running");
        return -1;
    }
    amqp_basic_qos(conn_, channel_id_, 0, 1, 0);
    auto queue_name = amqp_cstring_bytes(queue.c_str());
    amqp_basic_consume(conn_, channel_id_, queue_name, amqp_empty_bytes, 0, 0, 0, amqp_empty_table);
    auto reply = amqp_get_rpc_reply(conn_);
    if (reply.reply_type != AMQP_RESPONSE_NORMAL) {
        dzlog_error("consume failed");
        return errMessage(reply);
    }
    thread_ = thread(
        [this](function<void(const string &)> cb) {
            timeval timeout = {0, 200000}; // 200ms
            amqp_frame_t frame;
            auto future = promise_.get_future();
            while (future.wait_for(chrono::milliseconds(200)) == future_status::timeout) {
                amqp_maybe_release_buffers(conn_);
#ifdef LOW_LEVEL_API
                auto result = amqp_simple_wait_frame_noblock(conn_, &frame, &timeout);
                if (result != AMQP_STATUS_OK) {
                    if (result == AMQP_STATUS_TIMEOUT) {
                        dzlog_debug("amqp_simple_wait_frame_noblock timeout");
                        continue;
                    }
                    dzlog_error("amqp_simple_wait_frame failed");
                    errMessage((amqp_status_enum)result);
                    break;
                }
                dzlog_debug("frame type: %d channel: %d", frame.frame_type, frame.channel);
                if (frame.frame_type != AMQP_FRAME_METHOD) {
                    dzlog_debug("frame.frame_type != AMQP_FRAME_METHOD continue");
                    continue;
                }
                dzlog_debug("method: %s", amqp_method_name(frame.payload.method.id));
                if (frame.payload.method.id != AMQP_BASIC_DELIVER_METHOD) {
                    dzlog_debug("frame.payload.method.id != AMQP_BASIC_DELIVER_METHOD continue");
                    continue;
                }
                auto d = (amqp_basic_deliver_t *)frame.payload.method.decoded;
                dzlog_debug("delivery %u, exchange %.*s routingkey %.*s", (unsigned)d->delivery_tag,
                            (int)d->exchange.len, (char *)d->exchange.bytes, (int)d->routing_key.len,
                            (char *)d->routing_key.bytes);
                result = amqp_simple_wait_frame(conn_, &frame);
                if (result != AMQP_STATUS_OK) {
                    dzlog_error("amqp_simple_wait_frame failed");
                    errMessage((amqp_status_enum)result);
                    break;
                }
                if (frame.frame_type != AMQP_FRAME_HEADER) {
                    dzlog_fatal("expected header!");
                    break;
                }
                auto p = (amqp_basic_properties_t *)frame.payload.properties.decoded;
                if (p->_flags & AMQP_BASIC_CONTENT_TYPE_FLAG) {
                    dzlog_debug("content-type: %.*s", (int)p->content_type.len, (char *)p->content_type.bytes);
                }
                auto body_target = frame.payload.properties.body_size;
                uint64_t body_received = 0;
                string body_str;
                body_str.resize(body_target);
                while (body_received < body_target) {
                    result = amqp_simple_wait_frame(conn_, &frame);
                    if (result != AMQP_STATUS_OK) {
                        dzlog_error("amqp_simple_wait_frame failed");
                        errMessage((amqp_status_enum)result);
                        break;
                    }
                    if (frame.frame_type != AMQP_FRAME_BODY) {
                        dzlog_fatal("expected body!");
                        break;
                    }
                    memcpy(&body_str[body_received], frame.payload.body_fragment.bytes,
                           frame.payload.body_fragment.len);
                    body_received += frame.payload.body_fragment.len;
                }
                if (body_received != body_target) {
                    /* Can only happen when amqp_simple_wait_frame returns <= 0 */
                    /* We break here to close the connection */
                    break;
                }
                if (cb) {
                    cb(body_str);
                }
                if (amqp_basic_ack(conn_, channel_id_, d->delivery_tag, 0) == 0) {
                    dzlog_debug("acknowledged message %u", (unsigned)d->delivery_tag);
                } else {
                    dzlog_error("acknowledged message %u failed", (unsigned)d->delivery_tag);
                }
#else
                amqp_envelope_t envelope;
                auto result = amqp_consume_message(conn_, &envelope, &timeout, 0);
                if (result.reply_type != AMQP_RESPONSE_NORMAL) {
                    if (result.reply_type == AMQP_RESPONSE_LIBRARY_EXCEPTION &&
                        result.library_error == AMQP_STATUS_UNEXPECTED_STATE) {
                        if (amqp_simple_wait_frame(conn_, &frame) != AMQP_STATUS_OK) {
                            dzlog_error("amqp_simple_wait_frame failed");
                            break;
                        }
                        if (frame.frame_type == AMQP_FRAME_METHOD) {
                            switch (frame.payload.method.id) {
                            case AMQP_BASIC_ACK_METHOD:
                                /* if we've turned publisher confirms on, and we've published a
                                 * message here is a message being confirmed.
                                 */
                                dzlog_error("AMQP_BASIC_ACK_METHOD");
                                break;
                            case AMQP_BASIC_RETURN_METHOD:
                                /* if a published message couldn't be routed and the mandatory
                                 * flag was set this is what would be returned. The message then
                                 * needs to be read.
                                 */
                                {
                                    amqp_message_t message;
                                    auto r = amqp_read_message(conn_, frame.channel, &message, 0);
                                    if (AMQP_RESPONSE_NORMAL != r.reply_type) {
                                        dzlog_error("amqp_read_message failed");
                                        return;
                                    }
                                    amqp_destroy_message(&message);
                                }
                                dzlog_error("AMQP_BASIC_RETURN_METHOD");
                                break;

                            case AMQP_CHANNEL_CLOSE_METHOD:
                                /* a channel.close method happens when a channel exception occurs,
                                 * this can happen by publishing to an exchange that doesn't exist
                                 * for example.
                                 *
                                 * In this case you would need to open another channel redeclare
                                 * any queues that were declared auto-delete, and restart any
                                 * consumers that were attached to the previous channel.
                                 */
                                dzlog_error("AMQP_CHANNEL_CLOSE_METHOD");
                                dzlog_debug("consume thread exit");
                                return;

                            case AMQP_CONNECTION_CLOSE_METHOD:
                                /* a connection.close method happens when a connection exception
                                 * occurs, this can happen by trying to use a channel that isn't
                                 * open for example.
                                 *
                                 * In this case the whole connection must be restarted.
                                 */
                                dzlog_error("AMQP_CONNECTION_CLOSE_METHOD");
                                dzlog_debug("consume thread exit");
                                return;

                            default:
                                dzlog_error("An unexpected method was received %u", frame.payload.method.id);
                                dzlog_debug("consume thread exit");
                                return;
                            }
                        }
                    }
                    if (result.reply_type == AMQP_RESPONSE_LIBRARY_EXCEPTION &&
                        result.library_error == AMQP_STATUS_TIMEOUT) {
                        dzlog_debug("amqp_consume_message timeout");
                        continue;
                    }
                    dzlog_error("amqp_consume_message failed");
                    errMessage(result);
                    break;
                } else {
                    if (cb) {
                        cb(string((char *)envelope.message.body.bytes, envelope.message.body.len));
                    }
                    if (amqp_basic_ack(conn_, channel_id_, envelope.delivery_tag, 0) == 0) {
                        dzlog_debug("acknowledged message %u", (unsigned)envelope.delivery_tag);
                    } else {
                        dzlog_error("acknowledged message %u failed", (unsigned)envelope.delivery_tag);
                    }
                    amqp_destroy_envelope(&envelope);
                }
#endif
            }
            dzlog_debug("consume thread exit");
        },
        std::move(callback));
    return 0;
}
