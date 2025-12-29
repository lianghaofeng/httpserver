#include "work_stealing_pool.h"
#include <iostream>
#include <chrono>

WorkStealingPool::WorkStealingPool(size_t threads)
    : next_queue_(0), stop_(false){
    
    // 1. 创建n个工作队列
    for (size_t i = 0; i < threads; ++i){
        queues_.push_back(std::make_unique<WorkQueue>());
    }

    // 2. 启动n个工作线程
    for (size_t i = 0; i < threads; ++i){
        workers_.emplace_back([this, i]{
            worker_thread(i);
        });
    }
}

WorkStealingPool::~WorkStealingPool(){
    // 1. 设置停止标志
    stop_ = true;

    // 2. 等待所有线程结束
    for(std::thread& worker : workers_){
        if (worker.joinable()){
            worker.join();
        }
    }
}

// 工作线程主循环
void WorkStealingPool::worker_thread(size_t thread_id) {
    std::function<void()> task;

    while(!stop_){
        // 1. 从本地获取任务
        if(pop_local_task(thread_id, task)){
            task();
        } 
        // 2. 本地任务为空，尝试偷取任务
        else if(steal_task(thread_id, task)){
            task();
        }
        // 3. 偷取失败，等待
        else {
            // std::this_thread::sleep_for(std::chrono::milliseconds(1));
            std::this_thread::sleep_for(std::chrono::microseconds(10)); 
            // std::this_thread::yield();
        }
    }
}

bool WorkStealingPool::pop_local_task(size_t thread_id, std::function<void()>& task){
    std::unique_lock<std::mutex> lock(queues_[thread_id]->mutex);

    // 任务队列为空，返回false
    if(queues_[thread_id]->tasks.empty()){
        return false;
    }

    //从deque头部取出任务
    task = std::move(queues_[thread_id]->tasks.front());
    queues_[thread_id]->tasks.pop_front();

    return true;
}

bool WorkStealingPool::steal_task(size_t thief_id, std::function<void()>& task) {
    size_t n = queues_.size();
    size_t start = std::rand()%n;

    for(size_t i = 0; i < queues_.size(); ++i){
        size_t target = (start + i) % n;

        if(target == thief_id){
            continue;
        }

        std::unique_lock<std::mutex> lock(queues_[target]->mutex);

        if(!queues_[target]->tasks.empty()){
            //从尾偷取任务
            task = std::move(queues_[target]->tasks.back());
            queues_[target]->tasks.pop_back();
            return true;
        }
    }
    return false;
}

//添加任务
void WorkStealingPool::push_task(size_t thread_id, std::function<void()> task) {
    std::unique_lock<std::mutex> lock(queues_[thread_id]->mutex);
    queues_[thread_id]->tasks.push_back(std::move(task));
}