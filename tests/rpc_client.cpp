#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <sstream>
#include <algorithm>

#include "saxio/net.hpp"

using namespace saxio::net;

bool send_request(TcpStream& stream, const std::string& request) {
    // 线程局部的静态缓冲区，避免重复分配
    thread_local static std::vector<char> request_buffer;
    thread_local static std::vector<char> response_buffer(256);

    // 构造带换行符的请求
    request_buffer.assign(request.begin(), request.end());
    request_buffer.push_back('\n');

    // 发送请求
    if (auto wr = stream.write({request_buffer.data(), request_buffer.size()}); !wr) {
        std::cerr << "Write failed: " << wr.error().message() << std::endl;
        return false;
    }

    // 接收响应
    if (auto rd = stream.read({response_buffer.data(), response_buffer.size()}); !rd) {
        std::cerr << "Read failed: " << rd.error().message() << std::endl;
        return false;
    } else {
        // 只使用实际读取的字节数
        size_t bytes_received = rd.value();

        // 移除结尾的换行符
        while (bytes_received > 0 &&
               (response_buffer[bytes_received-1] == '\n' ||
                response_buffer[bytes_received-1] == '\r')) {
            bytes_received--;
        }

        std::string_view response(response_buffer.data(), bytes_received);
        std::cout << "请求: " << request << " -> 响应: " << response << std::endl;
    }
    return true;
}

void print_usage() {
    std::cout << "\n=== RPC 客户端使用说明 ===" << std::endl;
    std::cout << "支持的 RPC 调用格式:" << std::endl;
    std::cout << "  add <数字1> <数字2>    - 加法运算" << std::endl;
    std::cout << "  sub <数字1> <数字2>    - 减法运算" << std::endl;
    std::cout << "  mul <数字1> <数字2>    - 乘法运算" << std::endl;
    std::cout << "  div <数字1> <数字2>    - 除法运算" << std::endl;
    std::cout << "  quit                  - 退出客户端" << std::endl;
    std::cout << "  help                  - 显示帮助" << std::endl;
    std::cout << "示例: add 3 5, sub 10 4, mul 2 6, div 8 2" << std::endl;
    std::cout << "==========================\n" << std::endl;
}

bool validate_input(const std::string& input) {
    if (input == "quit" || input == "help") {
        return true;
    }

    std::istringstream iss(input);
    std::string command;
    std::string num1, num2;

    iss >> command >> num1 >> num2;

    if (command != "add" && command != "sub" && command != "mul" && command != "div") {
        std::cout << "错误: 不支持的命令 '" << command << "'" << std::endl;
        return false;
    }

    if (num1.empty() || num2.empty()) {
        std::cout << "错误: 参数不足，需要两个数字参数" << std::endl;
        return false;
    }

    // 检查是否为数字
    auto is_number = [](const std::string& s) {
        return !s.empty() && std::all_of(s.begin(), s.end(), [](char c) {
            return std::isdigit(c) || c == '-' || c == '+' || c == '.';
        });
    };

    if (!is_number(num1) || !is_number(num2)) {
        std::cout << "错误: 参数必须是数字" << std::endl;
        return false;
    }

    return true;
}

auto main() -> int {
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(8082);

    auto stream_result = TcpStream::connect_client(
        reinterpret_cast<sockaddr*>(&addr), sizeof(addr));

    if (!stream_result) {
        std::cerr << "连接服务器失败: " << stream_result.error().message() << std::endl;
        return -1;
    }

    auto& stream = *stream_result;
    std::cout << "成功连接到 RPC 服务器!" << std::endl;

    print_usage();

    std::string input;
    while (true) {
        std::cout << "RPC> ";
        std::getline(std::cin, input);

        // 处理空输入
        if (input.empty()) {
            continue;
        }

        // 处理帮助命令
        if (input == "help") {
            print_usage();
            continue;
        }

        // 处理退出命令
        if (input == "quit") {
            std::cout << "正在退出..." << std::endl;
            if (send_request(stream, "quit")) {
                std::cout << "已通知服务器关闭连接" << std::endl;
            }
            break;
        }

        // 验证输入格式
        if (!validate_input(input)) {
            std::cout << "请输入有效的 RPC 命令，或输入 'help' 查看帮助" << std::endl;
            continue;
        }

        // 发送请求
        if (!send_request(stream, input)) {
            std::cout << "与服务器的连接已断开" << std::endl;
            break;
        }
    }

    std::cout << "RPC 客户端已关闭" << std::endl;
    return 0;
}