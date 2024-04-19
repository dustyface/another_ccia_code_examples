#include <iostream>
#include <iterator>
#include <thread>
#include <functional>
#include "threadsafe_stack.h"

void test_threadsafe_stack() {
    int a[] = {1, 2, 3, 4, 5};
    ThreadSafeStack<int> stack(std::begin(a), std::end(a));
    auto lambda1 = [&stack] {
        try {
            std::cout << "thread_id=" << std::this_thread::get_id() << std::endl;
            // 使用返回值是shared_ptr的pop函数
            std::shared_ptr<int> value = stack.pop();
            std::cout << "do some stuff(lambda1) with popped value=" << *value << std::endl;
        } catch(const empty_stack& e) {
            std::cout << "empty stack(by lambda1)" << std::endl;
        }
    };
    std::function<void()> lambda2 = [&stack] {
        try {
            std::cout << "thread_id=" << std::this_thread::get_id() << std::endl;
            int value;
            stack.pop(value);
            std::cout << "do some stuff(lambda2) with popped value=" << value << std::endl;
        } catch(const empty_stack& e) {
            std::cout << "empty stack(by lambda2)" << std::endl;
        }
    };

    std::thread t1(lambda1);
    std::thread t2(lambda2);
    t1.join();
    t2.join();
    std::cout << "check stack size=" << stack.size() << std::endl;
}