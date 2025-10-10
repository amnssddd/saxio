#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <unordered_map>
#include <atomic>
#include <memory>
#include <csignal>
#include <fcntl.h>
#include <unistd.h>
#include "saxio/net.hpp"
#include "saxio/common/debug.hpp"
#include "saxio/net/tcp/stream.hpp"
#include "saxio/common/thread_pool.hpp"

using namespace saxio::net;

std::atomic<bool> server_running{true};
std::unique_ptr<ThreadPool> thread_pool;  //全局线程池

void process(const std::shared_ptr<TcpStream>& stream_ptr) {
    int client_fd = stream_ptr->fd();
    std::vector<char> buf(4096);
    LOG_INFO("Start processing client: {}", client_fd);

    while (true) {
        // 读取数据
        auto read_result = stream_ptr->read({buf.data(), buf.size()});
        if (!read_result) {
            if (read_result.error().value() == 0) {
                LOG_INFO("Client closed gracefully: {}", client_fd);
            } else {
                LOG_ERROR("Recv failed: {} - {}", client_fd, read_result.error());
            }
            break;
        }

        auto bytes_read = read_result.value();
        if (bytes_read == 0) {
            LOG_INFO("Client closed connection: {}", client_fd);
            break;
        }

        std::string_view received_data(buf.data(), bytes_read);

        // 处理空数据
        if (received_data.empty() ||
            (received_data.size() == 1 && received_data[0] == '\n')) {
            LOG_INFO("Received empty message from client: {}", client_fd);
            if (!stream_ptr->write("\n")) {
                LOG_DEBUG("Client disconnected during empty reply: {}", client_fd);
                break;
            }
            continue;
        }

        // 移除换行符
        if (!received_data.empty() && received_data.back() == '\n') {
            received_data.remove_suffix(1);
        }

        LOG_INFO("Response from client {} is: {}", client_fd, received_data);

        // 回传数据
        auto write_result = stream_ptr->write(received_data);
        if (!write_result) {
            LOG_ERROR("Failed to write data back to client {}: {}",
                     client_fd, write_result.error());
            break;
        }
    }

}

//实现服务端完美退出
auto signal_handler(int signal) -> void{
    if (signal == SIGINT) {
        //信号处理只做最简单的原子操作
        server_running = false;
    }
}

auto server() -> saxio::Result<void> {
    //初始化线程池：4个工作线程，最大等待100个任务
    thread_pool = std::make_unique<ThreadPool>(4, 100);
    LOG_INFO("Thread pool initializwd: 4 workers, max 100 pending tasks");

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(8080);

    auto has_listener = TcpListener::bind(
        reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
    if (!has_listener) {
        return std::unexpected{has_listener.error()};
    }

    auto tcp_listener = std::move(has_listener.value());
    LOG_INFO("Listening on port {}, fd is {}", ntohs(addr.sin_port), tcp_listener.fd());

    //设置监听 socket 为非阻塞模式
    int flags = fcntl(tcp_listener.fd(), F_GETFL, 0);
    if (flags == -1) {
        //致命错误，错误处理用return而不是LOG_ERROR
        return std::unexpected{make_error(saxio::Error::kSetNonBlockFailed)};
    }
    if (fcntl(tcp_listener.fd(), F_SETFL, flags | O_NONBLOCK) == -1) {
        return std::unexpected{make_error(saxio::Error::kSetNonBlockFailed)};
    }

    int accepted_count = 0;   //接收连接数
    int rejected_count = 0;   //拒接连接数

    while (server_running) {
        auto has_stream = tcp_listener.accept(nullptr, nullptr);
        if (!has_stream) {
            // 在非阻塞模式下，大多数错误只是"没有连接"，直接忽略非阻塞错误
            // 直接检查是否应该退出，然后继续循环
            if (!server_running) {
                LOG_INFO("Server shutdown initiated");
                break;
            }

            // 短暂休眠避免CPU空转
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }

        TcpStream stream(std::move(has_stream.value()));
        int client_fd = stream.fd();
        LOG_INFO("Connection accepted: {}", client_fd);

        //使用智能指针包装流对象（因为lambda表达式无法被复制）
        auto stream_ptr = std::make_shared<TcpStream>(std::move(stream));

        //提交任务到线程池（不再创建新线程）
        if (thread_pool->enqueue([stream_ptr]() {
            process(stream_ptr);
        })) {
            accepted_count++;
            LOG_INFO("Task enqueued successfully for clients {}, total accepted: {}",
                client_fd, accepted_count);
        }else {
            rejected_count++;
            LOG_WARN("Thread pool is full, rejecting connection from client {}. Total rejected: {}",
                client_fd, rejected_count);

            //发送“服务繁忙”响应给客户端
            auto ret = stream_ptr->write("Server busy, please try again later\n");
            if (!ret) {
                return std::unexpected{make_error(saxio::Error::kSendResponseFailed)};
            }
        }
    }

    //服务器关闭时，线程池会自动等待所有任务完成（RAII）
    LOG_INFO("Waiting for thread pool to complete all tasks...");
    thread_pool.reset();   //触发线程池析构，等待所有任务完成

    LOG_INFO("Server shutdown complete. Accepted: {}, Rejected: {}",
        accepted_count, rejected_count);
    return {};
}

auto main(int argc, char* argv[]) -> int {
    //注册信号处理器
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    LOG_INFO("Starting server... (Press Ctrl+C to stop)");

    try {
        auto ret = server();
        if (!ret) {
            LOG_ERROR("Server error: {}", ret.error());
            return -1;
        }
    } catch (const std::exception& e) {
        LOG_ERROR("Server exception: {}", e.what());
        return -1;
    }

    return 0;
}