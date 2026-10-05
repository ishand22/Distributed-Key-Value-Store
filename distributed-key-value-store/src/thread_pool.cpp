#include "dkv/thread_pool.hpp"
#include <algorithm>
#include <stdexcept>
namespace dkv {
ThreadPool::ThreadPool(std::size_t count){count=std::max<std::size_t>(1,count);for(std::size_t i=0;i<count;++i)workers_.emplace_back([this]{for(;;){std::function<void()> task;{std::unique_lock lock(mutex_);ready_.wait(lock,[this]{return stopping_||!tasks_.empty();});if(stopping_&&tasks_.empty())return;task=std::move(tasks_.front());tasks_.pop();}task();}});}
ThreadPool::~ThreadPool(){{std::lock_guard lock(mutex_);stopping_=true;}ready_.notify_all();for(auto& t:workers_)t.join();}
void ThreadPool::submit(std::function<void()> task){{std::lock_guard lock(mutex_);if(stopping_)throw std::runtime_error("thread pool is stopping");tasks_.push(std::move(task));}ready_.notify_one();}
}
