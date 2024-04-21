#include <iostream>
#include <thread>
#include <exception>
#include <unistd.h>
#include "thread_mgr.h"

const int TEST_VAL = 100;

struct Func {
    // if we reference a outer variable, which is a local variabl of a funciton.
    // the reference will be dangling after the function returns.
    int& ref_i;
    // 不可以用Func(const int&), 因为const int& 不允许bind 到ref_i
    Func(int& i_): ref_i(i_) {}

    // overload call operator
    void operator()() {
        std::cout << "Func::operator() start" << std::endl;
        const unsigned long max = 1e6;
        try {
            for (unsigned long i = 0; i < max; i++) {
                // 1. 测试检查ref_i在子线程的输出, 当main thread被调度执行完, ref_i成为了dangling reference
                // 由于在run()中子线程detach, 子线程在main thread执行完后，会继续执行，但实测中后续这段执行是不稳定的
                // 2. 提供output, 会拖慢程序的执行, Func callable执行慢，main thread有可能被调度到;
                // 当不提供output, 程序执行速度会快很多，main thread可能不会被调度到, 此时就不会出现dangling reference
                std::cout << "Func::operator() i=" << i << ", ref_i=" << ref_i << std::endl;
                // if (i == 1e2) {
                //     // 让main thread有机会被调度到
                //     std::cout << "Func::operator() sleep 1s" << std::endl;
                //     sleep(1);
                // }
                if (ref_i != TEST_VAL) {
                    std::cout << "Func::operator() ref_i is dangling" << std::endl;
                    std::cout << "Func::operator() ref_i=" << ref_i << std::endl;
                    break;
                }
            }
        } catch(std::exception e) {
            std::cerr << "Func::operator() exception: " << e.what() << std::endl;
        }
        std::cout << "Func::operator() end" << std::endl;
    }
};

void run() {
    std::cout << "run start" << std::endl;
    int local_state = TEST_VAL;
    Func my_func(local_state);      // Bad practice;
    std::thread my_thread(my_func);
    my_thread.detach();
    std::cout << "main thread sleep 1s" << std::endl;
    sleep(1);
    std::cout << "run end" << std::endl;
}
