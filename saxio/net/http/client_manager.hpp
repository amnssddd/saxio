#pragma once

#include <unordered_map>
#include <mutex>
#include <memory>
#include <thread>
#include <chrono>
#include <functional>

namespace saxio::http{

//客户端连接管理器，负责管理所有客户端连接的线程生命周期
class ClientManager {
public:
    //客户端线程消息结构体
    struct ClientThread {
        std::shared_ptr<std::thread> thread;   //客户端处理线程
        std::chrono::steady_clock::time_point start_time; //线程启动时间，用于超时处理
    };

    //添加新的客户端连接（使用模板更加通用）
    template<typename Callable>
    auto add_client(int fd, Callable&& handler) -> void{
        std::lock_guard<std::mutex> lock(clients_mutex_);

        ClientThread client_thread;
        client_thread.thread = std::make_shared<std::thread>(std::forward<Callable>(handler));
        client_thread.start_time = std::chrono::steady_clock::now();

        //try.emplace 会在容器内部直接构造对象，避免临时对象的创建和移动
        clients_.try_emplace(fd, std::move(client_thread));
    }

    //移除客户端连接
    auto remove_client(int fd) -> void{
        std::lock_guard<std::mutex> lock(clients_mutex_);
        if (auto it = clients_.find(fd); it != clients_.end()) {
            if (it->second.thread->joinable()) {
                it->second.thread->detach();  //分离线程，让系统自动回收
            }
            clients_.erase(it);
            LOG_DEBUG("Removed client: {}", fd);
        } else {
            LOG_DEBUG("Client {} not found for removal", fd);
        }
    }
    //清理已完成的客户端线程
    auto cleanup_finished_client() -> void{
        std::lock_guard<std::mutex> lock(clients_mutex_);
        for (auto it = clients_.begin(); it != clients_.end(); ) {
            if (it->second.thread->joinable()) {
                ++it;   //线程仍在运行，保留
            }else {
                it = clients_.erase(it);  //线程已完成，移除记录
            }
        }
    }

    //获取当前客户端数量
    auto client_count() const -> size_t{
        std::lock_guard<std::mutex> lock(clients_mutex_);
        return clients_.size();
    }

private:
    std::unordered_map<int, ClientThread> clients_;  //客户端线程映射表
    mutable std::mutex clients_mutex_;  //线程安全互斥锁（mutable允许在const成员函数中修改该变量）
};

}