#pragma once
#include <unistd.h>

namespace saxio::io::detail {
class FD {
public:
    explicit FD(int fd = -1)
        : fd_{fd}{}

    ~FD(){ close(); }

    //禁止拷贝
    FD(const FD&) = delete;
    FD& operator=(const FD&) = delete;

    //允许移动
    FD(FD&& other) noexcept{
        close();
        fd_ = other.fd_;
        other.fd_ = -1;
    }
    auto operator=(FD&& other) noexcept -> FD&{
        close();
        fd_ = other.fd_;
        other.fd_ = -1;
        return *this;
    }
public:
    [[nodiscard]]
    auto fd() const noexcept -> int { return fd_; }

    [[nodiscard]]
    auto is_valid() const noexcept -> bool { return fd_ >= 0; }

    [[nodiscard]]
    int release() noexcept{
        auto ret = fd_;
        fd_ = -1;
        return ret;
    }

    void close(){
        if (fd_ >= 0) {
            //可能会关闭失败，多关闭几次
            for (int i=0; i<3; i++) {
                if (::close(fd_) == 0) {
                    break;
                }
            }
        }
        fd_ = -1;
    }
protected:
    int fd_{-1};  //文件描述符初始化为-1
};
} // namespace saxio::io::detail
