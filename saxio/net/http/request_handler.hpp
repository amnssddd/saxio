#pragma once
#include "saxio/net/http/types.hpp"
#include "saxio/net/http/response_utils.hpp"
#include "saxio/net.hpp"
#include "saxio/common/debug.hpp"

namespace saxio::http{

//HTTP请求处理器，负责路由和业务逻辑处理
class RequestHandler {
public:
    //处理HTTP请求的主入口函数
    static auto handle_request(net::TcpStream& stream, const std::string& request) -> void {
        std::string path = parse_http_request(request);
        LOG_INFO("HTTP Request for path: {}", path);

        //路由分发
        if (path == "/" || path == "/index.html") {
            handle_root(stream);
        }else if (path == "/img.png") {
            handle_image(stream);
        }else if (path == "/favicon.ico"){
            //忽略favicon.ico请求，或者返回一个空的响应
            ResponseUtils::send_response_header(
                stream, HttpStatus::OK, "image/x-icon",0);
        }
        else {
            handle_not_found(stream);
        }
    }

private:
    //处理根路径请求，返回简历首页
    static auto handle_root(net::TcpStream& stream) -> void{
        const char* html = R"(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <title>我的简历</title>
    <style>
        body { font-family: Arial, sans-serif; margin: 40px; }
        h1 { color: #333; }
        img { max-width: 800px; border: 2px solid #ddd; border-radius: 8px; }
    </style>
</head>
<body>
    <h1>欢迎查看我的简历</h1>
    <p>点击下方图片查看完整简历：</p>
    <a href="/img.png">
        <img src="/img.png" alt="简历图片" style="max-width: 400px;">
    </a>
    <p><a href="/img.png">直接查看简历图片</a></p>
</body>
</html>
)";
        //发送HTTP响应头成功
        if (ResponseUtils::send_response_header(
            stream, HttpStatus::OK, get_mime_type(".html"), strlen(html))) {
            auto ret = stream.write(html);
            if (!ret) {
                LOG_ERROR("Send response header failed: {}" ,ret.error());
            }
        }
    }

    //处理图片请求，返回图片
    static auto handle_image(net::TcpStream& stream) -> void{
        std::string image_path = "/home/dinghaifeng/CLionProjects/saxio/doc/img.png";

        //检查文件是否存在
        std::ifstream test_file(image_path);
        if (!test_file) {
            LOG_ERROR("Image file not found: {}", image_path);
            //文件不存在直接进入404页面
            handle_not_found(stream);
            return;
        }
        test_file.close();

        //获取文件大小(ate:文件打开后文件指针会直接定位到文件的末尾，用于快速获取文件大小)
        std::ifstream file(image_path, std::ios::binary | std::ios::ate);
        size_t file_size = file.tellg();   //获取当前指针位置（即文件大小）
        file.close();

        LOG_INFO("Server image: {} (size: {} bytes)", image_path, file_size);

        //发送图片响应
        if (ResponseUtils::send_response_header(
            stream, HttpStatus::OK, get_mime_type(image_path), file_size)) {
            ResponseUtils::send_file_content(stream, image_path);
        }
    }

    //处理404未找到页面
    static auto handle_not_found(net::TcpStream& stream) -> void {
        const char* not_found_html = R"(
<!DOCTYPE html>
<html>
<head>
<meta charset="UTF-8">
<title>404 Not Found</title>
</head>
<body>
    <h1>404 - 页面未找到</h1>
    <p>请求的页面不存在</p>
    <p><a href="/">返回首页</a></p>
</body>
</html>
)";
        if (ResponseUtils::send_response_header(
                stream, HttpStatus::NOT_FOUND,
                get_mime_type(not_found_html), strlen(not_found_html))) {
            auto ret = stream.write(not_found_html);
            if (!ret){}
            LOG_ERROR("Send 404 response header error: {}", ret.error());
        }else {
            LOG_ERROR("Send 404 response head failed");
        }
    }

    //解析HTTP请求，提取请求路径
    static auto parse_http_request(const std::string& request) -> std::string{
        //简单的请求解析：提取第一个空格和第二个空格之间的路径
        size_t start = request.find(' ');
        if (start == std::string::npos) return "/";
        size_t end = request.find(' ', start+1);
        if (end == std::string::npos) return "/";

        return request.substr(start+1, end-start-1);  //提取路径
    }

};

}
