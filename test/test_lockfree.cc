#include <gtest/gtest.h>
#include "threadsafe_lockfree_stack.h"
#include "threadsafe_lockfree_queue_v2.h"

// test command:
// ./build/test_lockfree --gtest_filter=test_lockfree.test_refcount_stack
TEST(test_lockfree, test_refcount_stack) {
    std::cout << "stack start pushing..." << std::endl;
    std::thread t1([&] {
        int a[] = {5, 6, 7};
        push_items(a, 3);
    });
    std::thread t2([&] {
        int b[] = {2, 9};
        push_items(b, 2);
    });
    t1.join();
    t2.join();
    std::cout << "stack start poping..." << std::endl;
    std::thread t3([] {
        pop_items(1, true);
    });
    std::thread t4([] {
        pop_items(1, false);
    });
    t3.join();
    t4.join();
}

TEST(test_lockfree, struct_assign) {
    test_stucture_assign();
}

TEST(test_lockfree, test_refcount_queue) {
    std::cout << "queue start pushing..." << std::endl;
    std::thread ta([&] {
        int a[] = {100};
        push_queue_item(a, 1, true);
    });
    std::thread tb([&] {
        int b[] = {200};
        push_queue_item(b, 1, false);
    });
    ta.join();
    tb.join();
    std::cout << "queue start poping..." << std::endl;
    std::thread tc([&] {
        pop_queue_item(1);
    });
    std::thread td([&] {
        pop_queue_item(1);
    });
}