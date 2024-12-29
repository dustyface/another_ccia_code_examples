#include <gtest/gtest.h>
#include "threadsafe_lockfree_stack.h"

// test command: 
// ./build/test_lockfree --gtest_filter=test_lockfree.test_refcount_stack  
TEST(test_lockfree, test_refcount_stack) {
    std::cout << "start..." << std::endl;
    std::thread t1([&] {
        int a[] = {5, 6, 7, 8};
        push_items(a, 4);
    });
    std::thread t2([&] {
        int b[] = {2, 4, 9};
        push_items(b, 3);
    });
    std::cout << "start2..."  << std::endl;
    t1.join();
    t2.join();
    std::thread t3([] {
        std::cout << "thread_id=" << std::this_thread::get_id() << std::endl;
        pop_items(3);
    });
    std::thread t4([] {
        std::cout << "thread_id=" << std::this_thread::get_id() << std::endl;
        pop_items(2);
    });
    t3.join();
    t4.join();
}