#pragma once

#include <vector>
#include "saxio/common/error.hpp"

using namespace saxio;

//RPC函数实现
inline auto handle_add(double a, double b) -> double{ return a + b; }
inline auto handle_sub(double a, double b) -> double{ return a - b; }
inline auto handle_mul(double a, double b) -> double{ return a * b; }
inline auto handle_div(double a, double b) -> double{ return a / b; }

//RPC调用分发器
auto dispatch_rpc_call(std::string_view request) -> Result<int>{
    std::vector<std::string_view> parts;
    size_t start = 0;

    //分割字符串为3部分：[函数名，参数a，参数b]
    while (start < request.size()) {
        size_t end = request.find(' ', start);
        if (end == std::string_view::npos) {
            parts.push_back(request.substr(start));
            break;
        }
        //截取从start到end的子串并存入parts
        parts.push_back(request.substr(start, end-start));
        //更新start为下一个分割的起始位置
        start = end + 1;
    }
    //如果分割后不是3个部分直接返回错误
    if (parts.size() != 3) {
        return std::unexpected{make_error(Error::kRPCOutOfData)};
    }

    try {
        int a = std::stoi(std::string(parts[1]));
        int b = std::stoi(std::string(parts[2]));

        if (parts[0] == "add") {
            return handle_add(a, b);
        }else if (parts[0] == "sub") {
            return handle_sub(a, b);
        }else if (parts[0] == "mul") {
            return handle_mul(a, b);
        }else if (parts[0] == "div") {
            return handle_div(a, b);
        }

    }catch (const std::exception& e) {
        //参数转换（解析）失败
        return std::unexpected{make_error(Error::kRPCParameterParsingFailed)};
    }

    //未定义的函数调用
    return std::unexpected{make_error(Error::kRPCFindFunctionFailed)};
}