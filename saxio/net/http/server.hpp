#pragma once

#include <atomic>

#include "saxio/net/http/types.hpp"
#include "saxio/net/http/request_handler.hpp"
#include "saxio/net/http/client_manager.hpp"
#include "saxio/net.hpp"
#include "saxio/common/debug.hpp"
#include <vector>

namespace saxio::http{

//HTTP服务器主类，负责启动服务器和管理客户端连接
class Server {
public:
    //构造函数，指定服务器监听端口
    explicit Server(uint16_t port = 8090) : port_(port){
        LOG_INFO("HTTP Server initialized on port {}", port_);
    }

    //析构函数
    ~Server(){
        stop();
    }

    //启动HTTP服务器
    auto start() -> saxio::Result<void>{
        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = htonl(INADDR_ANY);
        addr.sin_port = htons(port_);

        //创建TCP套接字
        auto has_listener = net::TcpListener::bind(
            reinterpret_cast<sockaddr*>(&addr), sizeof(addr));

        if (!has_listener) {
            return std::unexpected{has_listener.error()};
        }

        auto tcp_listener = std::move(has_listener.value());  //value取出对象
        LOG_INFO("HTTP Server started on port {}, fd is {}",
            ntohs(addr.sin_port), tcp_listener.fd());

        //主服务器循环
        while (server_running_) {
            //只需要建立连接，不需要知道客户端信息，所以用nullptr
            auto has_stream = tcp_listener.accept(nullptr, nullptr);
            if (!has_stream) {
                LOG_ERROR("Accept failed: {}", has_stream.error());
                continue;   //单个连接失败不影响服务器运行
            }

            net::TcpStream stream(std::move(has_stream.value()));
            int client_fd = stream.fd();
            LOG_INFO("HTTP Connection accepted {}", client_fd);

            //清理已完成的客户端连接
            client_manager_.cleanup_finished_client();

            //为新客户端创建处理线程
            client_manager_.add_client(client_fd,
                [this, stream=std::move(stream)]() mutable {
                    this->process_client(std::move(stream));
                });

            LOG_INFO("HTTP client {} thread started, total clients: {}",
                client_fd, client_manager_.client_count());
        }
        return {};
    }

    //停止服务器
    auto stop() -> void{
        server_running_ = false;
    }

private:
    //处理单个客户端连接的函数
    auto process_client(net::TcpStream stream) -> void{
        thread_local std::vector<char> buf(4096);
        int client_fd = stream.fd();

        LOG_INFO("Start processing HTTP client: {}", client_fd);

        while (server_running_) {
            //读取客户端请求
            auto read_result = stream.read(
                {buf.data(), buf.size()});
            if (!read_result) {
                if (read_result.error().value() == 0) {
                    LOG_INFO("Client closed gracefully: {}", client_fd);
                }else {
                    LOG_ERROR("Recv failed: {} - {}", client_fd, read_result.error());
                }
                break;
            }

            auto bytes_read = read_result.value();
            if (bytes_read == 0) {
                LOG_INFO("Client closed connection: {}", client_fd);
                break;
            }

            std::string request{buf.data(), bytes_read};

            //清理请求显示：只显示第一行（请求行）
            size_t first_newline = request.find('\n');
            if (first_newline != std::string::npos) {
                std::string request_line = request.substr(0, first_newline);
                //移除回车符
                if (!request_line.empty() && request_line.back() == '\r') {
                    request_line.pop_back();
                }
                LOG_DEBUG("Received HTTP request from client {}: {}", client_fd, request_line);
            } else {
                LOG_DEBUG("Received HTTP request from client {}: {}", client_fd, request);
            }

            //处理HTTP请求
            RequestHandler::handle_request(stream, request);
            break;   //HTTP/1.0 简单处理，请求每个请求后关闭连接
        }

        //清理客户端资源
        client_manager_.remove_client(client_fd);
    }


    uint16_t port_;     //服务器监听端口
    std::atomic<bool> server_running_{true};  //服务器运行状态标志
    ClientManager client_manager_;      //客户端连接管理器
};

}
