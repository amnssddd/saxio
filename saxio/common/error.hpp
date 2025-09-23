#pragma once
#include <cstring>
#include <string_view>
#include <expected>

namespace saxio
{
    class Error {
    public:
        explicit Error(int error_code) : error_code_(error_code) {}

    public:
        enum ErrorCode {
            kUnknown = 8000,
            kBindFailed,
            kListenFailed,
            kSocketCreateFailed,
            kConnectFailed,
            kAcceptFailed,
        };

    public:
        [[nodiscard]]
        auto value() const noexcept -> int { return error_code_; }

        [[nodiscard]]
        auto message() const noexcept -> std::string_view{
            switch (error_code_) {
                case kUnknown:
                    return "Unknown error";
                case kBindFailed:
                    return "Bind failed";
                case kListenFailed:
                    return "Listen failed";
                case kSocketCreateFailed:
                    return "Socket create failed";
                case kConnectFailed:
                    return "Connect failed";
                case kAcceptFailed:
                    return "Accept failed";
                default:
                    //将错误码转换为可读的错误信息字符串
                    return strerror(error_code_);
            }
        }

    private:
        int error_code_;
    };

    template<typename T>
    using Result = std::expected<T, Error>;

    [[nodiscard]]
    static auto make_error(int error_code) -> Error{
        return Error(error_code);
    }
}
