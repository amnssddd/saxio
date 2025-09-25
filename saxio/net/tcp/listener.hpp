#pragma once

#include "saxio/net/tcp/stream.hpp"
#include "saxio/net/listener.hpp"

namespace saxio::net {
//监听端口并接收连接（accept()）,生成TcpStream
class TcpListener : public detail::BaseLinstener<TcpListener,TcpStream> {
public:
    explicit TcpListener(detail::Socket&& inner)
       : BaseLinstener(std::move(inner)){
    }
};
} // namespace saxio::net
