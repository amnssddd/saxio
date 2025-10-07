#pragma once

#include <string>
namespace saxio::http{

//HTTP响应状态码枚举
enum class HttpStatus {
    OK = 200,    //请求成功
    NOT_FOUND = 404,  //资源未找到
    INTERNAL_ERROR = 500,  //服务器内部错误
};

//HTTP请求结构体
struct HttpRequest {
    std::string method;   //请求方法（GET，POST等）
    std::string path;     //请求路径
    std::string version;  //HTTP 版本
};

//获取状态码的文本描述
inline auto get_status_text(const HttpStatus status) -> std::string {
    switch(status) {
        case HttpStatus::OK: return "OK";
        case HttpStatus::NOT_FOUND: return "Not Found";
        case HttpStatus::INTERNAL_ERROR: return "Internal Error";
        default: return "UNKNOWN";
    }
}

// 根据文件路径获取MIME类型(主类型/子类型)
inline auto get_mime_type(const std::string& path) -> std::string{
    //ends_with：比较字符串末尾是否和指定的后缀一致
    if (path.ends_with(".html") || path.ends_with(".htm")) return "text/html";
    if (path.ends_with(".jpg") || path.ends_with("jpeg")) return "image/jpeg";
    if (path.ends_with(".png")) return "image/png";
    if (path.ends_with(".gif")) return "image/gif";
    if (path.ends_with(".txt")) return "text/plain";
    if (path.ends_with(".css")) return "text/css";
    if (path.ends_with(".js")) return "application/javascript";
    return "application/octet-stream";  //默认的二进制流类型
}


}