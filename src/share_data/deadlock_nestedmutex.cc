#include <iostream>
#include <thread>
#include <mutex>
#include <unistd.h>
#include "share_data.h"

// netsted mutex cause deadlock in multiple threads case
X& X::operator=(const X& rhs) {
    std::cout << "thread_id=" << std::this_thread::get_id() << std::endl;
    if (this == &rhs)
        return *this;

    std::cout << "requesting lock for rhs(i=" <<  rhs.i << ")" << std::endl;
    std::lock_guard<std::mutex> lk_rhs(rhs.m);
    // 主动挂起线程, 让别的线程得到调度，就会复现deadlock
    sleep(1);
    std::cout << "requesting lock for lhs(i=" <<  i << ")" << std::endl;
    std::lock_guard<std::mutex> lk_lhs(m);
    i = rhs.i;
    return *this;
}

void X::set(int n) {
    i = n;
}

// thread initial funciton的参数是reference
// 需要注意，在定义thread时，传参需要std::ref, 否则编译不通过
void swap(X& lhs, X& rhs) {
    X temp = lhs;
    lhs = rhs;
    rhs = temp;
}
