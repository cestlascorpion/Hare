cmake_minimum_required(VERSION 3.16)

project(rabbit)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_CXX_FLAGS "-fPIC -Wall -Wextra -Wconversion -Wsizeof-pointer-memaccess \
                     -Wfloat-equal -Wconversion-null -Woverflow -Wshadow -faligned-new\
                     -D_FILE_OFFSET_BITS=64 -D_GNU_SOURCE -pthread -g -O0")

include_directories(src /usr/local/include)

link_directories(/usr/local/lib)

add_executable(rabbit
        main.cpp
        src/RabbitMQ.h
        src/RabbitMQ.cpp)

target_link_libraries(rabbit zlog amqpcpp rabbitmq)
