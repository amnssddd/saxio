#include <iostream>
#include <tbb/tbb.h>
#include <tbb/concurrent_hash_map.h>

#include "saxio/net.hpp"
#include "saxio/log/logger.hpp"
#include "saxio/common/debug.hpp"
using namespace saxio::net;

// 线程安全的客户端容器
tbb::concurrent_hash_map<int, tbb::task_group> clients;

auto process(const TcpStream &stream) {
    char buf[1024];
    while(true) {
        auto len = ::recv(stream.fd(), buf, sizeof(buf), 0);
        if (len <= 0) {
            if (len == 0) {
                LOG_INFO("Connection closed: {}", stream.fd());
            } else {
                LOG_ERROR("Connection error: {}", strerror(errno));
            }
            break;
        }

        buf[len] = '\0';
        std::cout << "Received: " << buf;

        len = ::send(stream.fd(), buf, len, 0);
        if (len <= 0) {
            if (len == 0) {
                LOG_INFO("Connection closed: {}", stream.fd());
            } else {
                LOG_ERROR("Connection error: {}", strerror(errno));
            }
            break;
        }
    }

    // 安全删除客户端
    tbb::concurrent_hash_map<int, tbb::task_group>::accessor acc;
    if (clients.find(acc, stream.fd())) {
        acc->second.wait();  // 等待任务完成
        clients.erase(acc);  // 线程安全删除
    }
}

auto server() -> saxio::Result<void> {
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(8082);

    auto has_listener = TcpListener::bind(reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
    if (!has_listener) {
        return std::unexpected{has_listener.error()};
    }

    auto tcp_listener = std::move(has_listener.value());
    LOG_INFO("Listening on port {}, fd is {}", addr.sin_port, tcp_listener.fd());

    // 创建TBB任务调度区
    tbb::task_arena arena(4);  // 使用4个工作线程

    sockaddr_in clnt_addr{};
    socklen_t clnt_addrlen{};

    while (true) {
        auto has_stream = tcp_listener.accept(reinterpret_cast<sockaddr*>(&clnt_addr), &clnt_addrlen);
        if (!has_stream) {
            return std::unexpected{has_stream.error()};
        }

        LOG_INFO("Connection accepted: {}", has_stream.value().fd());

        // 使用TBB任务组管理客户端线程
        arena.execute([&] {
            tbb::concurrent_hash_map<int, tbb::task_group>::accessor acc;
            clients.insert(acc, has_stream.value().fd());  // 安全插入
            acc->second.run([stream = std::move(has_stream.value())] {
                process(stream);
            });
        });
    }
}

auto main(int argc, char* argv[]) -> int {
    // 初始化TBB
    tbb::global_control global_limit(tbb::global_control::max_allowed_parallelism, 4);

    auto ret = server();
    if (!ret) {
        LOG_ERROR("server error: {}", ret.error());
    }

    // 等待所有客户端任务完成
    for (auto& [fd, tg] : clients) {
        tg.wait();
    }

    return 0;
}