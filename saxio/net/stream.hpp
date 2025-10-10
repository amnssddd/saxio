#pragma once

#include "saxio/net/socket.hpp"
#include "saxio/io/impl/impl_read.hpp"
#include "saxio/io/impl/impl_write.hpp"

namespace saxio::net::detail {
template <class Stream>
class BaseStream : public io::detail::ImplRead<Stream>,
                   public io::detail::ImplWrite<Stream>{
public:
    explicit BaseStream(Socket&& inner)
        : inner_(std::move(inner)){}

public:
    //获取文件描述符
    [[nodiscard]]
    auto fd() const noexcept -> int{ return inner_.fd(); }

    //关闭连接
    void close(){ inner_.close(); }

public:
    //静态方法：建立连接
    [[nodiscard]]
    static auto connect(const sockaddr* serv_addr, socklen_t serv_addrlen) -> Result<Stream>{
        //创建 Socket
        auto has_socket = Socket::create(AF_INET, SOCK_STREAM, 0);
        if (!has_socket) {
            return std::unexpected{ has_socket.error()};
        }

        //绑定地址（此处绑定到默认地址）
        auto socket = std::move(has_socket.value());  //value()用于提取内部存储的成功值（Socket）
        sockaddr_in addr{};
        socklen_t addrlen = 0;
        auto has_bind = socket.bind(reinterpret_cast<const sockaddr*>(&addr), addrlen);
        if (!has_bind) {
            return std::unexpected{make_error(Error::kBindFailed)};
        }
        //返回 Stream 对象（具体类型由模板参数 Stream 决定）
        return Stream{std::move(socket)};
    }

private:
    Socket inner_;  //内部持有的 socket
};
}
