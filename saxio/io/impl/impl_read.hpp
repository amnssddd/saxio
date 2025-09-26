#pragma once

#include <span>

#include "saxio/io/io.hpp"

namespace saxio::io::detail{

template <class T>
struct ImplRead {
    [[nodiscard]]
    auto read(std::span<char> buf) const noexcept -> Result<std::size_t>{
        auto ret = ::read(static_cast<const T*>(this)->fd(),
            buf.data(), buf.size());
        if (ret >= 0) return ret;
        return std::unexpected{make_error(Error::kReadFailed)};
    }
};

}
