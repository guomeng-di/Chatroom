#pragma once
#include <condition_variable>
#include <cstddef>
#include <deque>
#include <functional>
#include <mutex>
#include <thread>
#include <vector>

class TaskThreadPool{
public:
    static TaskThreadPool& instance();
    void enqueue(std::function<void()> task);

private:
    TaskThreadPool();
    ~TaskThreadPool();
    TaskThreadPool(const TaskThreadPool&)=delete;
    TaskThreadPool& operator=(const TaskThreadPool&)=delete;
    void workerLoop();

    bool stopping_;
    std::mutex mutex_;
    std::condition_variable condition_;
    std::deque<std::function<void()>> tasks_;
    std::vector<std::thread> workers_;
};
