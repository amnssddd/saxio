#pragma once

#include "saxio/net/socket.hpp"
#include "saxio/net/stream.hpp"

namespace saxio::net {
class TcpStream : public detail::BaseStream<TcpStream> {
public:
    explicit TcpStream(detail::Socket&& inner)
        : BaseStream<TcpStream>(std::move(inner)){}
};
}