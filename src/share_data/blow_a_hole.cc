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
    template <typename F>
    void process_data(F func) {
        std::lock_guard<std::mutex> lk(m_);
        func(data_);
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