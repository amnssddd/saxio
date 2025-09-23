#pragma once

#include "saxio/net/socket.hpp"

namespace saxio::net::detail {

template <class Listener, class Stream>
class BaseLinstener {
public:
    explicit BaseLinstener(Socket&& inner)
        : inner_(std::move(inner)){}

public:
    [[nodiscard]]
    auto accept(sockaddr* addr, socklen_t* addrlen) -> Result<Stream>{
        auto clnt_fd = ::accept(fd(), addr, addrlen);
        if (clnt_fd < 0) {
            return std::unexpected{make_error(Error::kAcceptFailed)};
        }
        return Stream{Socket{clnt_fd}};
    }

public:
    [[nodiscard]]
    auto fd() const noexcept -> int{ return inner_.fd(); }

    [[nodiscard]]
    static auto bind(const sockaddr* addr, socklen_t addrlen) -> Result<Listener>{
        // Create
        auto has_socket = Socket::create(AF_INET, SOCK_STREAM, 0);
        if (!has_socket) {
            return std::unexpected{has_socket.error()};
        }
        auto socket = std::move(has_socket.value());

        // Bind
        auto has_bind = socket.bind(addr, addrlen);
        if (!has_bind) {
            std::cerr << strerror(errno) << std::endl;
            return std::unexpected{has_bind.error()};
        }

        // Listen
        auto has_listen = socket.listen(SOMAXCONN);
        if (!has_listen) {
            return std::unexpected{has_listen.error()};
        }
        return Listener{std::move(socket)};
    }

private:
    Socket inner_;
};

}

