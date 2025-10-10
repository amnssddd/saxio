#pragma once
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <functional>
#include <condition_variable>
#include <atomic>


namespace saxio::net
{

class ThreadPool {
public:
    /**
     * @brief 创建指定数量的工作线程
     * @param num_threads 工作线程数量
     * @param max_pending 最大等待任务数，用于控制并发连接数
     */
    explicit ThreadPool(size_t num_threads, size_t max_pending = 100) :
        max_pending_tasks_(max_pending), stop_(false){
        //创建指定数量的工作线程
        for (size_t i = 0; i < num_threads; ++i) {
            workers_.emplace_back([this] {
                //工作线程的主循环
                while (true) {
                    std::function<void()> task;
                    {
                        //等待任务或停止信号
                        std::unique_lock<std::mutex> lock(queue_mutex_);
                        condition_.wait(lock, [this] {
                            return stop_ || !tasks_.empty();  //条件：停止或有任务
                        });

                        //检查是否需要退出
                        if (stop_ && tasks_.empty()) return;

                        //获取任务
                        task = std::move(tasks_.front());
                        tasks_.pop();

                    }
                    //执行任务（在锁外执行，避免阻塞其他线程）
                    task();
                }
            });
        }
    }

    /**
     * @brief 提交任务到线程池
     * @param task 要执行的任务函数
     * @return true-提交成功，false-队列已满（用于拒绝新连接）
     */
    auto enqueue(std::function<void()> task) -> bool{
        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            //检查队列是否已满
            if (tasks_.size() >= max_pending_tasks_) {
                return false;
            }

            //任务入队
            tasks_.push(std::move(task));
        }
        //通知一个等待的工作线程
        condition_.notify_one();
        return true;
    }

    //析构用于优雅地关闭线程池（RAII）
    ~ThreadPool(){
        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            stop_ = true;  //设置停止标志
        }
        //通知所有工作线程
        condition_.notify_all();
        //等待所有工作线程结束
        for (std::thread &worker : workers_) {
            if (worker.joinable()) worker.join();
        }
    }

private:
    std::vector<std::thread> workers_;          //工作线程集合
    std::queue<std::function<void()>> tasks_;   //任务队列
    std::mutex queue_mutex_;                    //保护任务队列的互斥锁
    std::condition_variable condition_;         //线程间通信的条件变量
    std::atomic<bool> stop_;                    //停止标志（原子操作）
    const size_t max_pending_tasks_;            //最大等待任务数
};

}// namespace saxio
