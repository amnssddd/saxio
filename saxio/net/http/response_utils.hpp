#pragma once

#include "saxio/net/http/types.hpp"
#include "saxio/net.hpp"
#include "saxio/common/debug.hpp"
#include <sstream>
#include <fstream>
#include <vector>

namespace saxio::http{

//HTTP响应工具类，负责生成和发送HTTP响应
class ResponseUtils {
public:
    //发送HTTP响应头
    static auto send_response_header(net::TcpStream& stream,    //TCP流，代表与客户端的网络连接
                                    HttpStatus status,          //HTTP响应码
                                    const std::string& content_type,     //响应内容MIME类型
                                    size_t content_length = 0) -> bool{  //响应主体（body）长度
        std::ostringstream header;
        header << "HTTP/1.1 " << static_cast<int>(status)
               << " " << get_status_text(status) << "\r\n";
        header << "Content-Type: " << content_type << "\r\n";
        if (content_length > 0) {
            header << "Content-Length: " << content_length << "\r\n";
        }
        header << "Connection: close\r\n";
        header << "\r\n";  //空行分隔头部和主体

        std::string header_str = header.str();
        auto result = stream.write(header_str);
        if (!result) {
            LOG_ERROR("Failed to send response header: {}", result.error());
            return false;
        }
        return true;
    }

    //发送文件内容到客户端
    static auto send_file_content(net::TcpStream& stream,
                                  const std::string& file_path) -> bool{
        std::ifstream file(file_path, std::ios::binary);  //以二进制模式打开文件
        //文件不存在
        if (!file) {
            LOG_ERROR("Failed to open file: {}", file_path);
            return false;
        }

        //线程局部静态缓冲区，每个线程只初始化一次
        thread_local std::vector<char> buffer(4096);

        //使用缓冲区流式传输文件内容
        while (file.read(buffer.data(), static_cast<std::streamsize>(buffer.size()))) {
            //发送读取的数据（gcount为从文件实际读取的字节数，表示每次读取发送4096字节）
            auto result = stream.write({buffer.data(),
                                                            static_cast<size_t>(file.gcount())});
            if (!result) {
                LOG_ERROR("Failed to send data: {}", result.error());
                return false;
            }
        }

        //当剩余数据小于缓冲区大小时会退出循环，所以需发送最后一块数据
        if (file.gcount() > 0) {
            auto result = stream.write({buffer.data(),
                                                            static_cast<size_t>(file.gcount())});
            if (!result) {
                LOG_ERROR("Failed to send final data: {}", result.error());
                return false;
            }
        }
        return true;
    }

};

}
