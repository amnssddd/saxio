#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <unordered_map>
#include "saxio/net.hpp"
#include "saxio/log/logger.hpp"
#include "saxio/common/debug.hpp"
#include "saxio/net/tcp/stream.hpp"

using namespace saxio::net;

// 线程安全的客户端容器
std::unordered_map<int, std::thread> clients;
std::mutex clients_mutex;

auto process(TcpStream &stream){
    std::vector<char> buf(4096);

    while (true) {
        //读取数据
        auto read_result = stream.read(
            {buf.data(), buf.size()});
        if (!read_result) {
            if (read_result.error().value() == 0) {
                LOG_INFO("Client closed gracefully: {}", stream.fd());
            } else {
                LOG_ERROR("Recv failed: {}", read_result.error());
            }
            break;
        }

        //处理数据
        auto bytes_read = read_result.value(); //获取Result<>的值size_t
        // 客户端主动关闭时会返回0长度数据（需注意CRTL C属于强制关闭，会触发下面的信息）
        if (bytes_read == 0) {
            LOG_INFO("Client closed connection: {}", stream.fd());
            break;
        }
        std::string_view received_data(buf.data(), bytes_read);

        //处理空数据
        if (received_data.empty() ||
            received_data.size() == 1 && received_data[0] == '\n') {
            LOG_INFO("Received empty message");
            if (!stream.write("\n")) {
                //尝试回复心跳
                LOG_DEBUG("Client disconnected during empty reply");
                break; //写入失败说明连接已断开
            }
            continue;
        }

        //移除换行符
        if (!received_data.empty() && received_data.back() == '\n') {
            received_data.remove_suffix(1);
        }

        //打印回复消息
        LOG_INFO("Response is: {}", received_data);

        //回传数据
        auto write_result = stream.write(received_data);
        if (!write_result) {
            LOG_ERROR("Failed to write data back to client {}: {}",
                      stream.fd(),
                      write_result.error());
            break;
        }
    }
    //清理客户端
    std::lock_guard<std::mutex> lock(clients_mutex);
    if (auto it = clients.find(stream.fd()); it != clients.end()) {
        it->second.join();
        clients.erase(it);
    }
}

auto server() -> saxio::Result<void>{
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(8082);

    auto has_listener = TcpListener::bind(
        reinterpret_cast<sockaddr *>(&addr), sizeof(addr));
    if (!has_listener) {
        return std::unexpected{has_listener.error()};
    }

    auto tcp_listener = std::move(has_listener.value());
    LOG_INFO("Listening on port {}, fd is {}", addr.sin_port, tcp_listener.fd());

    while (true) {
        auto has_stream = tcp_listener.accept(nullptr, nullptr);
        if (!has_stream) {
            return std::unexpected{has_stream.error()};
        }
        LOG_INFO("Connection accepted: {}", has_stream.value().fd());

        TcpStream stream(std::move(has_stream.value()));
        //启动新线程处理连接
        std::lock_guard<std::mutex> lock(clients_mutex);
        clients.emplace(
            stream.fd(),
            std::thread([stream=std::move(stream)]()mutable {
                process(stream);
            })
        );
    }
}

auto main(int argc, char *argv[]) -> int{
    auto ret = server();
    if (!ret) {
        LOG_ERROR("server error: {}", ret.error());
    }
    return 0;
}
