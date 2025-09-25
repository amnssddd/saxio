#include <iostream>
#include <tbb/tbb.h>
#include <tbb/concurrent_hash_map.h>

#include "saxio/net.hpp"
#include "saxio/log/logger.hpp"
#include "saxio/common/debug.hpp"
#include "saxio/net/socketIO.hpp"
using namespace saxio::net;

// 线程安全的客户端容器
tbb::concurrent_hash_map<int, tbb::task_group> clients;

auto process(const TcpStream &stream) {
    socketIO io(stream.fd());
    std::vector<char> buf;

    while(true) {
        try {
            //读取数据（自动处理缓冲区）
            buf = io.readAll();  // 读取失败会抛出异常

            //回显数据
            std::string_view response(buf.data(), buf.size());

            // 处理空消息或仅换行符
            if (response.empty() || (response.size() == 1 && response[0] == '\n')) {
                LOG_DEBUG("Received empty message");
                if (io.write("\n") <= 0) break;  // 回显换行符
                continue;
            }

            // 安全移除换行符
            if (!response.empty() && response.back() == '\n') {
                response.remove_suffix(1);
            }

            LOG_INFO("Received: {}", response);
            if (io.write(response) <= 0) break;  // 发送失败退出
        }
        catch(const std::system_error& e){
            if (e.code().value() == 0) {  // 客户端主动关闭
                LOG_INFO("Client closed gracefully: {}", stream.fd());
            } else if (e.code().value() == ECONNRESET) {  // 客户端强制断开
                LOG_WARN("Client reset connection: {}", stream.fd());
            } else {
                LOG_ERROR("IO error: {}", e.what());
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
    return 0;
}