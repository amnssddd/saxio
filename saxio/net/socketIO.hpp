#pragma once

#include <vector>
#include <string_view>
#include <system_error>
#include <sys/socket.h>

namespace saxio::net{

class socketIO {
public:
    explicit socketIO(int fd) : fd_(fd) {}

    // 读取数据到缓冲区（vector<char>为动态缓冲区，可自动管理内存并调整数据大小）
    [[nodiscard]]
    auto read(std::vector<char>& buf) const -> ssize_t{
        buf.resize(1024);  // 默认缓冲区大小
        ssize_t len = ::recv(fd_, buf.data(), buf.size(), 0);
        if (len > 0) buf.resize(len);
        return len;
    }

    // 发送字符串数据（std::string_view为数据的视图，可避免不必要的字符拷贝）
    [[nodiscard]]
    auto write(std::string_view data) const -> ssize_t{
        return ::send(fd_, data.data(), data.size(), 0);
    }

    // 带错误处理的读取（抛出异常）
    [[nodiscard]]
    auto readAll() const -> std::vector<char>{
        std::vector<char> buf;
        ssize_t len = read(buf);
        if (len == 0) {
            throw std::system_error(0, std::system_category(), "client closed");  // 自定义错误码
        } else if (len < 0) {
            throw std::system_error(errno, std::system_category(), "recv failed");
        }
        return buf;  // 空消息也一并返回
    }

    [[nodiscard]]
    int fd() const { return fd_; }

private:
    int fd_;
};

}

