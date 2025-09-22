#pragma once

#include "saxio/net/socket.hpp"

namespace saxio::net::detail {

template <typename Listener, class Stream>
class BaseLinstener {
public:
    explicit BaseLinstener(Socket&& inner)
        : _inner(std::move(inner)){}

public:
    [[nodiscard]]
    auto accept(sockaddr* addr, socklen_t* addrlen) -> Result<Stream>{
        auto clnt_fd = ::accept(fd(), addr, addrlen);
    }

protected:
    Socket _inner;

};

}

