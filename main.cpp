//
//  main.cpp
//  ThreadPool
//
//  Created by Yiping Wang on 2023/10/25.
//

#include <iostream>
#include "ThreadPool2.hpp"
#include <mutex>
#include <vector>
#include <atomic>

std::mutex m;

std::atomic<int> num = 0;

class A {
public:
    static void print(){
//        std::cout << "Hello" << std::endl;
    }
    void pp() {
//        std::cout << "pp" << std::endl;
    }
    explicit A(int v) {
//        std::cout << "A()" << std::endl;
        this->x = new int;
        *(this->x) = v;
    }
    A(const A &a) {
        num ++;
        std::cout << "+";
//        std::cout << "A(A&)" << std::endl;
        this->x = new int;
        *(this->x) = *(a.x);
    }
    A(A &&a) noexcept {
//        std::cout << "A(A&&)" << std::endl;
        this->x = a.x;
        a.x = nullptr;
    }
    ~A() {
//        std::cout << "~A()" << std::endl;
        delete this->x;
    }
    int *x;
};
/*
void mainx() {
    std::cout << "Hello, Let's begin" << std::endl;
    ThreadPool tp(10);
    std::vector<std::future<A>> res;
    for(int i = 0; i < 20; i ++) {
        res.push_back(tp.submit([] (int i, A a)-> A {
            std::this_thread::sleep_for(std::chrono::seconds(3));
            {
                std::unique_lock<std::mutex> lk(m);
                std::cout << "I am " << i << std::endl;
            }
            *(a.x) = i;
            return a;
        }, i * i, A(10)));
    }
    tp.addThreads(10);
    for(int i = 20; i < 40; i ++) {
        res.push_back(tp.submit([] (int i, A a)-> A {
            std::this_thread::sleep_for(std::chrono::seconds(3));
            {
                std::unique_lock<std::mutex> lk(m);
                std::cout << "I am " << i << std::endl;
            }
            *(a.x) = i;
            return a;
        }, i * i, A(31)));
    }
    std::cout << "all over" << std::endl;
    for(auto & i : res) {
        std::cout << *((i.get()).x) << std::endl;
    }
}

*/

void mainy() {
    ThreadPool2 tp(30);
    std::cout << "begin" << std::endl;
//    for(int i = 0; i < 10; i ++) tp.submit([i]() {
//        std::cout << "+";
//        std::this_thread::sleep_for(std::chrono::seconds(1));
//        std::cout << "-";
//    });
//    std::this_thread::sleep_for(std::chrono::seconds(7));
    for(int i = 0; i < 30000; i ++) tp.submit([i](A a) {
//        std::cout << "+";
        std::this_thread::sleep_for(std::chrono::seconds(1));
//        std::cout << "-";
    }, A(10));
    std::this_thread::sleep_for(std::chrono::seconds(5));
    tp.submit([](A a) {
//        std::cout << "+";
        std::this_thread::sleep_for(std::chrono::seconds(1));
//        std::cout << "-";
    }, A(12));
    std::this_thread::sleep_for(std::chrono::seconds(5));
    for(int i = 0; i < 30000; i ++) tp.submit([i](A a) {
//        std::cout << "+";
        std::this_thread::sleep_for(std::chrono::seconds(1));
//        std::cout << "-";
    }, A(10));
    std::this_thread::sleep_for(std::chrono::seconds(5));
    std::cout << num << std::endl;
}

int main() {
    mainy();
}
