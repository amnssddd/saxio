#pragma once

#include "saxio/net/tcp/stream.hpp"
#include "saxio/net/listener.hpp"

namespace saxio::net {
class TcpListener : public detail::BaseLinstener<TcpListener,TcpStream> {
    explicit TcpListener(detail::Socket&& inner)
        : BaseLinstener(std::move(inner)) {}
};
} // namespace saxio::net
