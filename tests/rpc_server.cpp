#include "saxio/net.hpp"
#include "saxio/net/rpc/rpc_handle.hpp"
#include "saxio/log/logger.hpp"
#include "saxio/common/debug.hpp"
#include "saxio/net/tcp/stream.hpp"
#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <unordered_map>
#include <atomic>
#include <memory>


using namespace saxio::net;

// 使用智能指针管理线程
std::unordered_map<int, std::shared_ptr<std::thread>> clients;
std::mutex clients_mutex;
std::atomic<bool> server_running{true};

void process(TcpStream stream) {
    std::vector<char> buf(4096);
    int client_fd = stream.fd();

    LOG_INFO("Start processing RPC client: {}", client_fd);

    while (true) {
        // 读取数据
        auto read_result = stream.read({buf.data(), buf.size()});
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
            if (!stream.write("\n")) {
                LOG_DEBUG("Client disconnected during empty reply: {}", client_fd);
                break;
            }
            continue;
        }

        // 移除换行符
        if (!received_data.empty() && received_data.back() == '\n') {
            received_data.remove_suffix(1);
        }

        // 处理退出连接
        if (received_data == "quit") {
            LOG_INFO("Client requested quit: {}", client_fd);
            break;
        }

        // 处理RPC请求
        LOG_INFO("Received RPC request from client {}: {}", client_fd, received_data);
        auto result = saxio::rpc::dispatch_rpc_call(received_data);

        if (result) {
            std::string response = std::to_string(result.value());
            // 发送成功响应
            if (auto wr = stream.write(std::string_view(response)); !wr) {
                LOG_ERROR("Failed to send response to client {}: {}", client_fd, wr.error());
                break;
            }
            LOG_INFO("Sent response to client {}: {}", client_fd, response);
        } else {
            std::string_view response = "ERROR";
            // 发送错误响应
            if (auto wr = stream.write(response); !wr) {
                LOG_ERROR("Failed to send error response to client {}: {}", client_fd, wr.error());
                break;
            }
            LOG_INFO("Sent error response to client {}: {}", client_fd, response);
        }
    }

    // 清理客户端
    std::lock_guard<std::mutex> lock(clients_mutex);
    if (auto it = clients.find(client_fd); it != clients.end()) {
        if (it->second->joinable()) {
            it->second->detach();
        }
        clients.erase(it);
        LOG_INFO("RPC client {} cleaned up, remaining clients: {}", client_fd, clients.size());
    }
}

void cleanup_finished_clients() {
    std::lock_guard<std::mutex> lock(clients_mutex);
    for (auto it = clients.begin(); it != clients.end(); ) {
        if (it->second->joinable()) {
            ++it;
        } else {
            LOG_DEBUG("Removing finished RPC client: {}", it->first);
            it = clients.erase(it);
        }
    }
}

auto server() -> saxio::Result<void> {
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(8082);

    auto has_listener = TcpListener::bind(
        reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
    if (!has_listener) {
        return std::unexpected{has_listener.error()};
    }

    auto tcp_listener = std::move(has_listener.value());
    LOG_INFO("RPC Server listening on port {}, fd is {}", ntohs(addr.sin_port), tcp_listener.fd());

    while (server_running) {
        auto has_stream = tcp_listener.accept(nullptr, nullptr);
        if (!has_stream) {
            LOG_ERROR("Accept failed: {}", has_stream.error());
            continue;
        }

        TcpStream stream(std::move(has_stream.value()));
        int client_fd = stream.fd();
        LOG_INFO("RPC Connection accepted: {}", client_fd);

        // 定期清理已完成的客户端
        cleanup_finished_clients();

        // 启动新线程处理连接
        std::lock_guard<std::mutex> lock(clients_mutex);

        auto client_thread = std::make_shared<std::thread>([stream = std::move(stream)]() mutable {
            process(std::move(stream));
        });

        clients.emplace(client_fd, client_thread);
        LOG_INFO("RPC client {} thread started, total clients: {}", client_fd, clients.size());
    }

    // 服务器关闭时等待所有客户端线程结束
    std::lock_guard<std::mutex> lock(clients_mutex);
    for (auto& [fd, thread_ptr] : clients) {
        if (thread_ptr->joinable()) {
            thread_ptr->join();
        }
    }
    clients.clear();

    return {};
}

auto main(int argc, char* argv[]) -> int {
    try {
        LOG_INFO("Starting RPC Server...");
        auto ret = server();
        if (!ret) {
            LOG_ERROR("RPC Server error: {}", ret.error());
            return -1;
        }
    } catch (const std::exception& e) {
        LOG_ERROR("RPC Server exception: {}", e.what());
        return -1;
    }

    return 0;
}