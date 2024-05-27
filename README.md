# RabbitMQ Demo

RABBITMQ IS VERY BAD FOR ME. I HATE RABBITMQ.

ref: <https://github.com/liuhaobupt/rabbitmq_work_queues_demo-with-rabbit-c-client-lib.git>

`class RabbitMQClient` -> do everything except `Publish()`/`Consume()`;

`class RabbitMQPublisher` -> use `RabbitMQClient`'s method to make itself ready and then call `Publish()`(thread safe).

`class RabbitMQConsumer` -> user `RabbitMQClient`'s method to make itself ready and then call `Consume()`(only once). A thread is create to do recvLoop thing.

I don't think it's necessary to write a `class RabbitMQManager` or things like that.
