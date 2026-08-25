#include "TaskThreadPool.h"
#include "../Logger/Logger.h"

TaskThreadPool& TaskThreadPool::instance(){
    static TaskThreadPool pool;
    return pool;
}

TaskThreadPool::TaskThreadPool():stopping_(false){
    const size_t workerCount=4;
    workers_.reserve(workerCount);
    for(size_t i=0;i<workerCount;i++){
        workers_.emplace_back(&TaskThreadPool::workerLoop,this);
    }
    LOG_INFO<<"创建业务任务线程池成功 线程数量="<<workerCount;
}

TaskThreadPool::~TaskThreadPool(){
    {
        std::lock_guard<std::mutex> lock(mutex_);
        stopping_=true;
    }
    condition_.notify_all();
    for(auto& worker:workers_){
        if(worker.joinable()) worker.join();
    }
    LOG_INFO<<"业务任务线程池已停止";
}

void TaskThreadPool::enqueue(std::function<void()> task){
    if(!task) return;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if(stopping_) return;
        tasks_.push_back(std::move(task));
    }
    condition_.notify_one();
}

void TaskThreadPool::workerLoop(){
    while(true){
        std::function<void()> task;
        {
            std::unique_lock<std::mutex> lock(mutex_);
            condition_.wait(lock,[this](){
                return stopping_||!tasks_.empty();
            });
            if(stopping_&&tasks_.empty()) return;
            task=std::move(tasks_.front());
            tasks_.pop_front();
        }
        try{
            task();
        }catch(const std::exception& e){
            LOG_ERROR<<"业务任务执行异常:"<<e.what();
        }catch(...){
            LOG_ERROR<<"业务任务执行未知异常";
        }
    }
}
