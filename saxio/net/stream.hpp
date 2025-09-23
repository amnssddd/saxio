#pragma once

#include "saxio/net/socket.hpp"

namespace saxio::net::detail {
template <class Stream>
class BaseStream {
public:
    explicit BaseStream(Socket&& inner)
        : inner_(std::move(inner)){}

public:
    [[nodiscard]]
    auto fd() const noexcept -> int{ return inner_.fd(); }

    void close(){ inner_.close(); }

public:
    [[nodiscard]]
    static auto connect(const sockaddr* serv_addr, socklen_t serv_addrlen) -> Result<Stream>{
        //Create
        auto has_socket = Socket::create(AF_INET, SOCK_STREAM, 0);
        if (!has_socket) {
            return std::unexpected{has_socket.error()};
        }
        auto socket = std::move(has_socket.value());

        //Bind
        sockaddr_in addr{};
        socklen_t addrlen = 0;
        auto has_bind = socket.bind(reinterpret_cast<const sockaddr*>(&addr), addrlen);
        if (!has_bind) {
            return std::unexpected{make_error(Error::kBindFailed)};
        }
        return Stream{std::move(socket)};
    }

protected:
    Socket inner_;
};
}
