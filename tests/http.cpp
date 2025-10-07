#include "saxio/net/http/server.hpp"

auto main() -> int{
    try {
        //创建HTTP服务器示例，监听8090端口
        saxio::http::Server server(8090);
        LOG_INFO("Starting HTTP server...");

        //启动服务器
        auto ret = server.start();
        if (!ret) {
            LOG_ERROR("HTTP server error: {}", ret.error());
        }
    }catch (const std::exception& e) {
        LOG_ERROR("HTTP Server exception: {}", e.what());
        return -1;
    }

    return 0;
}