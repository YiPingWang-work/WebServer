//
//  ThreadPool.hpp
//  ThreadPool
//
//  Created by Yiping Wang on 2023/10/25.
//

#ifndef ThreadPool_hpp
#define ThreadPool_hpp

#include <mutex>
#include <vector>
#include <queue>
#include <functional>
#include <future>
#include <atomic>
#include <iostream>
#include <semaphore>

class ThreadPool {
public:
    explicit ThreadPool(int maxThreadNum = 10, int appanding = 0):
    _over(false), _taskNum(0), _maxThreadNum(0), _appanding(appanding), _MAX_THREAD_NUM(4000) {
        this -> addThreads(maxThreadNum < _MAX_THREAD_NUM ? maxThreadNum : _MAX_THREAD_NUM);
        this -> _guade = std::thread([this](){
            for(;;) {
                std::unique_lock<std::mutex> lk(this -> _m2);
                this -> _adding.wait(lk, [this](){
                    return this -> _taskNum > this -> _maxThreadNum || this -> _over;
                });
                if(this -> _taskNum <= this -> _maxThreadNum && this -> _over) return;
                if(this -> _taskNum > this -> _maxThreadNum)
                    this -> addThreads(appending(this -> _appanding));
            }
        });
    }
    virtual ~ThreadPool() {
        this -> _over = true;
        this -> _adding.notify_one();
        this -> _guade.join();
        this -> _cv.notify_all();
        for(auto & i : this -> _threads) {
            i.join();
        }
    };
    ThreadPool(const ThreadPool &) = delete;
    ThreadPool(ThreadPool &&) = delete;
public:
    template<class _Func, class ... _Args>
    auto submit(_Func&& _func, _Args&& ... _args) -> std::future<decltype(_func(std::forward<_Args>(_args) ...))> {
        ++ this -> _taskNum;
        while(this -> _taskNum > this -> _maxThreadNum && this -> _maxThreadNum < this -> _MAX_THREAD_NUM) this -> _adding.notify_one();
        ThreadPool *tp = this;
        auto taskPtr = std::make_shared<
                std::packaged_task<decltype(_func(std::forward<_Args>(_args) ...))(_Args&& ...)>
                > (std::forward<_Func>(_func));
        {
            std::unique_lock<std::mutex> lk(tp -> _m);
            if(tp -> _over) throw std::runtime_error("submit on stopped ThreadPool");
            tp -> _tasks.push([taskPtr, ... _args = std::forward<_Args>(_args)]() mutable {
                (*taskPtr)(std::forward<_Args>(_args) ...);
            });
        }
        tp -> _cv.notify_one();
        return taskPtr -> get_future();
    }
    int size() {
        return this -> _maxThreadNum;
    }
    void shutdown() {
        this -> _over = true;
    }
private:
    int appending(int num = 0) {
        if(num <= 0) return this -> _maxThreadNum;
        else return num;
    }
    void addThreads(int num) {
        if(this -> _maxThreadNum + num > _MAX_THREAD_NUM) {
            num = _MAX_THREAD_NUM - this -> _maxThreadNum;
        }
        if(num < 0) throw std::runtime_error("illigal number of threads");
        if(num == 0) return;
        std::cout << "adding" << std::endl;
        for(int i = 0; i < num; ++ i) {
            this -> _threads.push_back(std::thread([this, tid = i]() {
                for(;;) {
                    std::function<void()> f;
                    {
                        std::unique_lock<std::mutex> lk(this ->_m);
                        this ->_cv.wait(lk, [this]() {
                            return !this -> _tasks.empty() || this -> _over;
                        });
                        if(this -> _over && this -> _tasks.empty()) return;
                        f = std::move((this->_tasks).front());
                        this ->_tasks.pop();
                    }
                    f();
                    -- this -> _taskNum;
                }
            }));
        }
        this -> _maxThreadNum += num;
    }
private:
    bool _over;
    std::condition_variable _cv;
    std::mutex _m; // 锁_tasks
    std::queue<std::function<void()>> _tasks;
    std::vector<std::thread> _threads;
private:
    std::mutex _m2;
    std::condition_variable _adding;
    std::atomic<int> _taskNum;
    std::atomic<int> _maxThreadNum;
    std::thread _guade;
    int _MAX_THREAD_NUM;
    int _appanding;
};


#endif /* ThreadPool_hpp */
