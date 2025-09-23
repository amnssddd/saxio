#include <iostream>
#include <thread>
#include <unordered_map>

#include "saxio/net.hpp"
using namespace saxio::net;

std::unordered_map<int, std::thread> clients;

auto process(const TcpStream &stream){
    char buf[1024];
    while(true) {
        auto len = ::recv(stream.fd(), buf, sizeof(buf), 0);
        if (len<=0) {
            if (len == 0) {
                std::cout << "Connection closed: " << stream.fd() << std::endl;
            }else {
                std::cout << "Connection error: " << strerror(errno) << std::endl;
            }
            break;
        }
        buf[len] = '\0';
        std::cout << "Received: " << buf;
        len = ::send(stream.fd(), buf, len, 0);
        if (len<=0) {
            if (len == 0) {
                std::cout << "Connection closed: " << stream.fd() << std::endl;
            }else {
                std::cout << "Connection error: " << stream.fd() << std::endl;
            }
            break;
        }
    }
    //读写关闭后将客户端从容器中移除
    clients.erase(stream.fd());
}

auto server() -> saxio::Result<void>{
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(8082);
    auto has_listener = TcpListener::bind(reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
    if (!has_listener) {
        return std::unexpected{has_listener.error()};
    }

    auto tcp_listener = std::move(has_listener.value());
    std::cout << "Listening on: " << tcp_listener.fd() << std::endl;
    sockaddr_in clnt_addr{};
    socklen_t clnt_addrlen{};
    while (true) {
        auto has_stream = tcp_listener.accept(reinterpret_cast<sockaddr*>(&clnt_addr), &clnt_addrlen);
        if (!has_stream) {
            return std::unexpected{has_stream.error()};
        }
        std::cout << "Connection accepted: " << has_stream.value().fd() << std::endl;

        //线程不安全
        std::thread t(process, std::move(has_stream.value()));
        t.detach();
    }
}

auto main(int argc, char* argv[]) -> int{
    auto ret = server();
    if (!ret) {
        std::cerr << ret.error().message() << std::endl;
    }
    return 0;
}