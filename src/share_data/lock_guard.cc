#include <iostream>
#include <list>
#include <algorithm>
#include <mutex>
#include <thread>

std::list<int> some_list;
std::mutex some_mutex;

// std::lock_guard + std::mutex 是被推荐的RAII风格的使用mutex的方式
void add_to_list(int new_value) {
    std::lock_guard<std::mutex> lk(some_mutex);
    some_list.push_back(new_value);
}

bool list_contain(int value_to_find) {
    std::lock_guard<std::mutex> lk(some_mutex);
    return std::find(some_list.begin(), some_list.end(), value_to_find) != some_list.end();
}

void test_lock_guard() {
    std::thread t1([] {
        std::cout << "thread t=" << std::this_thread::get_id() << std::endl;
        for (int i = 0; i < 100; i++) {
            add_to_list(i);
        }
    });
    t1.detach();
    while (true) {
        std::cout << "looking for 50" << std::endl;
        if (list_contain(50)) {
            std::cout << "find 50" << std::endl;
            break;
        }
    }
}