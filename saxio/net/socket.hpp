#pragma once

#include "saxio/io/io.hpp"
#include "saxio/common/error.hpp"
#include <arpa/inet.h>

namespace saxio::net::detail {
class Socket : public io::detail::FD {
public:
    explicit Socket(int fd = -1) : FD(fd) {}

public:
    [[nodiscard]]
    auto bind(const sockaddr* addr, socklen_t addrlen) const noexcept -> Result<void>{
        if (::bind(fd_, addr, addrlen) < 0) {  //bind失败
            return std::unexpected{make_error(Error::kBindFailed)};
        }
        return saxio::Result<void>{};  //成功时无返回值
    }

    [[nodiscard]]
    auto listen(int maxn) const noexcept -> saxio::Result<void>{
        if (::listen(fd_, maxn) < 0) {
            return std::unexpected{make_error(Error::kListenFailed)};
        }
        return saxio::Result<void>{};
    }

    [[nodiscard]]
    static auto create(const int domain, const int type, const int protocol)->Result<Socket>{
        auto fd = ::socket(domain, type, protocol);
        if (fd < 0) {
            return std::unexpected{make_error(Error::kSocketCreateFailed)};
        }
        return Socket{fd};
    }
};
}