#include <thread>
#include "threadsafe_lockfree_stack.h"

lock_free_stack<int> stack;

struct counted_node_ptr {
        int external_count;
        int* ptr;
    };

void push_items(int items[], size_t size) {
    for (int i = 0; i < size; i++) {
        stack.push(items[i]);
    }
}

void pop_items(int size, bool use_pause) {
    for (int i = 0; i < size; i++) {
        std::cout << "pop_items: i=" << i << std::endl;
        std::shared_ptr<int> res = stack.pop(use_pause);
        if (res)
            std::cout << "poped item=" << *res << std::endl;
    }
}

void test_stucture_assign() {
    counted_node_ptr old_counter;
    old_counter.external_count = 1;
    old_counter.ptr = new int(10);
    counted_node_ptr new_counter;
    new_counter = old_counter;
    std::cout << "old_counter.ptr=" << old_counter.ptr
        << ", old_counter.external_counter=" << old_counter.external_count
        << ", new_counter=" << new_counter.ptr
        << ", new_counter.external_counter=" << new_counter.external_count
        << std::endl;
    delete old_counter.ptr;
}