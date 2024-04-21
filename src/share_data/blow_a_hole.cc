#include <iostream>
#include <thread>
#include <mutex>
#include "share_data.h"

struct some_data {
    int a = 0;
    std::string b = "";
    void do_something(int aa, std::string bb) {
        if (a == 0 && b == "") {
            a = aa;
            b = bb;
        }
    }
};

class data_wrapper {
private:
    some_data data_;
    std::mutex m_;
public:
    template <typename Func>
    void process_data(Func func) {
        std::lock_guard<std::mutex> lk(m_);
        func(data_);
    }
    template <typename Func>
    std::unique_lock<std::mutex> process_data_with_lock(Func func) {
        std::unique_lock<std::mutex> lk(m_);
        func(data_);
        return lk;
    }
};

some_data* unprotected;
void malicious_function(some_data& data) {
    unprotected = &data;
}

void test_mutex_hole() {
    data_wrapper x;
    std::thread t1([&x] {
        x.process_data(malicious_function);
        unprotected->do_something(100, "devil");
    });
    std::thread t2([&x] {
        x.process_data(malicious_function);
        unprotected->do_something(200, "angel");
    });
    t1.join();
    t2.join();
    std::cout << "check unprotected data: a=" << unprotected->a << ", b=" << unprotected->b << std::endl;
}

void test_unqiue_lock() {
    data_wrapper x;
    auto lambda_func = [&x](int a, std::string b) {
        // process_data_with_lock 和 do_something 都在mutex保护下
        std::unique_lock<std::mutex> lk(x.process_data_with_lock(malicious_function));
        unprotected->do_something(a, b);
    };
    std::thread t1(lambda_func, 100, "devil");
    std::thread t2(lambda_func, 200, "angel");
    t1.join();
    t2.join();
    std::cout << "check unprotected data: a=" << unprotected->a << ", b=" << unprotected->b << std::endl;
}