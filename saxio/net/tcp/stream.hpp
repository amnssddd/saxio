#pragma once

#include "saxio/net/socket.hpp"
#include "saxio/net/stream.hpp"

namespace saxio::net {
//CRTP（奇异递归模板模式）：派生类将自身类型作为模板参数传递给基类
//管理一个已建立的连接
class TcpStream : public detail::BaseStream<TcpStream> {
public:
    //构造函数：调用基类构造函数（构造函数仅初始化基类（传递socket的所有权），其它功能由基类提供）
    explicit TcpStream(detail::Socket&& inner)
       : BaseStream<TcpStream>(std::move(inner)){}

    //客户端连接方法
    [[nodiscard]]
    static auto connect_client(const sockaddr* serv_addr, socklen_t serv_addrlen)
        -> Result<TcpStream>{
        //创建套接字
        auto has_socket = detail::Socket::create(AF_INET, SOCK_STREAM, 0);
        if (!has_socket) {
            return std::unexpected{has_socket.error()};
        }
        auto socket = std::move(has_socket.value());

        //连接服务器
        if (::connect(socket.fd(), serv_addr, serv_addrlen) < 0) {
            return std::unexpected{make_error(Error::kClientConnectFailed)};
        }
        return TcpStream{std::move(socket)};
    }
};
}