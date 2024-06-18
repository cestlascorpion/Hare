#include <rabbitmq-c/amqp.h>
#include <rabbitmq-c/tcp_socket.h>

#include <string>

namespace Rabbit {

class RabbitMQClient {
public:
    RabbitMQClient(const std::string& host, uint16_t port, const std::string& user, const std::string& password);

    virtual ~RabbitMQClient();

    RabbitMQClient(const RabbitMQClient&) = delete;
    RabbitMQClient& operator=(const RabbitMQClient&) = delete;

public:
    /**
     * @brief Connects to a RabbitMQ server. thread-unsafe.
     *
     * This function establishes a connection to a RabbitMQ server using the specified virtual host.
     * If no virtual host is provided, the default virtual host ("/") will be used.
     *
     * @param vhost The virtual host to connect to. Default is "/".
     *
     * @return return 0 if success, otherwise return -1.
     */
    int Connect(const std::string& vhost = "/");
    /**
     * @brief Declares a new exchange on the RabbitMQ server. thread-unsafe.
     *
     * This function is used to declare a new exchange with the specified parameters on the RabbitMQ server.
     *
     * @param exchange The name of the exchange to declare.
     * @param type The type of the exchange (e.g., "direct", "topic", "fanout").
     * @param passive If set to true, the server will only check if the exchange exists without creating it.
     * @param durable If set to true, the exchange will survive server restarts.
     * @param auto_delete If set to true, the exchange will be deleted when no longer in use.
     * @param internal If set to true, the exchange will be used for internal purposes and cannot be published to
     * directly.
     *
     * @return return 0 if success, otherwise return -1.
     */
    int ExchangeDeclare(const std::string& exchange, const std::string& type, bool passive, bool durable,
                        bool auto_delete, bool internal);
    /**
     * @brief Declares a queue in RabbitMQ. thread-unsafe.
     *
     * This function declares a queue with the specified parameters in RabbitMQ.
     *
     * @param queue The name of the queue to declare.
     * @param passive If set to true, the server will reply with an error if the queue already exists.
     * @param durable If set to true, the queue will survive a server restart.
     * @param auto_delete If set to true, the queue will be deleted when there are no more consumers.
     * @param internal If set to true, the queue will be used only for exchange-to-exchange bindings and cannot be
     * consumed from.
     *
     * @return return 0 if success, otherwise return -1.
     */
    int QueueDeclare(const std::string& queue, bool passive, bool durable, bool auto_delete, bool internal);
    /**
     * @brief Binds a queue to an exchange with a specified binding key. thread-unsafe.
     *
     * @param queue The name of the queue to bind.
     * @param exchange The name of the exchange to bind to.
     * @param binding_key The binding key used for the binding.
     *
     * @return return 0 if success, otherwise return -1.
     */
    int QueueBind(const std::string& queue, const std::string& exchange, const std::string& binding_key);
    /**
     * @brief Unbinds a queue from an exchange with a specific binding key. thread-unsafe.
     *
     * This function unbinds the specified `queue` from the specified `exchange` using the provided `binding_key`.
     *
     * @param queue The name of the queue to unbind.
     * @param exchange The name of the exchange to unbind from.
     * @param binding_key The binding key used for the unbinding.
     *
     * @return return 0 if success, otherwise return -1.
     */
    int QueueUnbind(const std::string& queue, const std::string& exchange, const std::string& binding_key);
    /**
     * @brief Deletes a queue from RabbitMQ. thread-unsafe.
     *
     * This function deletes a queue from RabbitMQ based on the provided queue name.
     *
     * @param queue The name of the queue to be deleted.
     * @param if_unused If set to true, the queue will only be deleted if it is unused.
     * @param if_empty If set to true, the queue will only be deleted if it is empty.
     *
     * @return return 0 if success, otherwise return -1.
     */
    int QueueDelete(const std::string& queue, bool if_unused, bool if_empty);

protected:
    static int errMessage(amqp_status_enum status);
    static int errMessage(amqp_rpc_reply_t_ reply);

protected:
    const std::string host_;
    const uint16_t port_;
    const std::string user_;
    const std::string password_;
    amqp_channel_t channel_id_; // one channel for one connection, so it == 1 when client's ready.
    amqp_socket_t* socket_;
    amqp_connection_state_t conn_;
};

class RabbitMQPublisher : public RabbitMQClient {
public:
    RabbitMQPublisher(const std::string& host, uint16_t port, const std::string& user, const std::string& password);
    ~RabbitMQPublisher() override;

public:
    /**
     * @brief Publishes a message to a RabbitMQ exchange with the specified routing key. thread-unsafe.
     *
     * NOTE: ExchangeDeclare(), QueueDeclare() AND QueueBind() should be called before calling this function.
     *
     * @param exchange The name of the exchange to publish the message to.
     * @param routing_key The routing key used to route the message to the appropriate queue(s).
     * @param message The message to be published.
     *
     * @return return 0 if success, otherwise return -1.
     */
    int Publish(const std::string& exchange, const std::string& routing_key, const std::string& message);
};

class RabbitMQConsumer : public RabbitMQClient {
public:
    RabbitMQConsumer(const std::string& host, uint16_t port, const std::string& user, const std::string& password);
    ~RabbitMQConsumer() override;

public:
    /**
     * @brief Prepares the RabbitMQ for a specific queue. thread-unsafe.
     *
     * This function prepares the RabbitMQ for a specific queue by performing necessary setup operations.
     *
     * @param queue The name of the queue to prepare.
     * @return An integer value indicating the success or failure of the preparation process.
     */
    int Prepare(const std::string& queue);
    /**
     * @brief Consumes a message from the RabbitMQ queue. thread-unsafe.
     *
     * This function consumes a message from the RabbitMQ queue and stores it in the provided string.
     *
     * @param message The string to store the consumed message.
     * @param timeout The maximum time to wait for a message to be consumed.
     *
     * @return return 0 if success or timeout, otherwise return -1.
     */
    int Consume(std::string& message, timeval timeout);
};

} // namespace Rabbit