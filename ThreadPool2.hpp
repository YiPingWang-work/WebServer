//
//  ThreadPool2.hpp
//  ThreadPool
//
//  Created by Yiping Wang on 2023/10/26.
//

#ifndef ThreadPool2_hpp
#define ThreadPool2_hpp

#include <mutex>
#include <list>
#include <queue>
#include <functional>
#include <future>
#include <atomic>
#include <iostream>
#include <semaphore>

class ThreadPool2 {
public:
    explicit ThreadPool2(int maxThreadNum = 10, int appanding = 0):
    _over(false),_guadeOver(false), _del(false), _taskNum(0), _maxThreadNum(0), _appanding(appanding), _MAX_THREAD_NUM(4000), _addOrDel(0) {
        _addThreads(maxThreadNum < _MAX_THREAD_NUM ? maxThreadNum : _MAX_THREAD_NUM);
        _guade = std::thread([this](){
            for(;;) {
                _addOrDel.acquire();
                if(!_tooFew() && _guadeOver) return;
                if(_tooFew()) _addThreads(_append(_appanding));
                if(_tooMany()) {
                    std::cout << "bye" << std::endl;
                    _del = true;
                    int ts = (int)_threads.size() / 2;
                    for(int i = 0; i < ts; i ++) _cv.notify_one();
                    std::this_thread::sleep_for(std::chrono::microseconds(20));
                    _del = false;
                }
                {
                    std::unique_lock<std::mutex> lk(_m2);
                    for(;_deadThreads.size();) {
                        auto & dead = _deadThreads.front();
                        dead -> join();
                        ++ _cnt2;
                        _threads.erase(std::find(_threads.begin(), _threads.end(), dead));
                        _deadThreads.pop_front();
                        delete dead;
                    }
                }
            }
        });
    }
    virtual ~ThreadPool2() {
        _guadeOver = true;
        _addOrDel.release();
        _guade.join();
        _over = true;
        _cv.notify_all();
        std::cout << "waiting" << std::endl;
        for(auto & i : _threads) {
            i -> join();
            delete i;
            _cnt2 ++;
        }
        std::cout << "====" << _cnt1 << "====" << _cnt2 << std::endl;
    };
    ThreadPool2(const ThreadPool2 &) = delete;
    ThreadPool2(ThreadPool2 &&) = delete;
public:
    template<class _Func, class ... _Args>
    auto submit(_Func&& _func, _Args&& ... _args) -> std::future<decltype(_func(std::forward<_Args>(_args) ...))> {
        ++ _taskNum;
        if((_tooFew() || _tooMany()) && !_over) _addOrDel.release();
        auto taskPtr = std::make_shared<
                std::packaged_task<decltype(_func(std::forward<_Args>(_args) ...))(_Args&& ...)>
            > (std::forward<_Func>(_func));
        {
            std::unique_lock<std::mutex> lk(_m);
            if(_over) throw std::runtime_error("submit on stopped ThreadPool");
            _tasks.push([taskPtr, ... _args = std::forward<_Args>(_args)]() mutable {
                (*taskPtr)(std::forward<_Args>(_args) ...);
            });
        }
        _cv.notify_one();
        return taskPtr -> get_future();
    }
    int size() {
        return _maxThreadNum;
    }
    void shutdown() {
        _over = true;
    }
private:
    int _append(int num = 0) {
        if(num <= 0) return _maxThreadNum;
        else return num;
    }
    void _addThreads(int num) {
        if(_maxThreadNum + num > _MAX_THREAD_NUM) {
            num = _MAX_THREAD_NUM - _maxThreadNum;
        }
        if(num < 0) throw std::runtime_error("illigal number of threads");
        if(num == 0) return;
        for(int i = 0; i < num; ++ i) {
            ++ _cnt1;
            std::promise<std::thread*> threadPtrPromise;
            std::future<std::thread*> threadPtrFuture = threadPtrPromise.get_future();
            std::thread* threadPtr = new std::thread([this](std::future<std::thread*> && threadPtr) mutable {
                for(;;) {
                    std::function<void()> f;
                    {
                        std::unique_lock<std::mutex> lk(_m);
                        _cv.wait(lk, [this]() {
                            return !_tasks.empty() || _over || _del;
                        });
                        if(_over && _tasks.empty()) return;
                        if(_del && _tasks.empty()) goto __DEL__;
                        f = std::move(_tasks.front());
                        _tasks.pop();
                    }
                    f();
                    -- _taskNum;
                }
                __DEL__:
                -- _maxThreadNum;
                {
                    std::unique_lock<std::mutex> lk(_m2);
                    _deadThreads.push_back(threadPtr.get());
                }
        }, std::move(threadPtrFuture));
            threadPtrPromise.set_value(threadPtr);
            _threads.push_back(threadPtr);
        }
        _maxThreadNum += num;
    }
    bool _tooFew() {
        return _taskNum > _maxThreadNum && _maxThreadNum < _MAX_THREAD_NUM;
    }
    bool _tooMany() {
        return _taskNum < _maxThreadNum / 2 && _maxThreadNum > _MAX_THREAD_NUM / 2;
    }
private:
    std::atomic<int> _cnt1 = 0;
    std::atomic<int> _cnt2 = 0;
    std::atomic<bool> _over;
    std::atomic<bool> _guadeOver;
    std::atomic<bool> _del;
    std::condition_variable _cv;
    std::mutex _m; // 锁_tasks
    std::queue<std::function<void()>> _tasks;
    std::list<std::thread*> _threads;
    std::list<std::thread*> _deadThreads;
    std::mutex _m2;
    std::binary_semaphore _addOrDel;
    std::atomic<int> _taskNum;
    std::atomic<int> _maxThreadNum;
    std::thread _guade;
    int _MAX_THREAD_NUM;
    int _appanding;
};


#endif /* ThreadPool2_hpp */
