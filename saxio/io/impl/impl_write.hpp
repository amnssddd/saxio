#pragma once

#include <span>

#include "saxio/io/io.hpp"

namespace saxio::io::detail{

template <class T>
struct ImplWrite {
    [[nodiscard]]
    auto write(const std::span<const char> buf) noexcept -> Result<std::size_t>{
        auto ret = ::write(static_cast<const T*>(this)->fd(),
            buf.data(), buf.size());
        if (ret >= 0) return ret;
        return std::unexpected{make_error(Error::kWriteFailed)};
    }

    //提供一个专门处理字符串字面量的 write() 重载
    [[nodiscard]]
    auto write(const char* str) noexcept -> Result<std::size_t>{
        return this->write(std::span<const char>{str, strlen(str)});
    }
};

}