#include <iostream>
#include <stack>
#include "share_data.h"

ThreadUnsafeStack::ThreadUnsafeStack(int* item, size_t size) {
    for (size_t i = 0; i < size; i++) {
        stack_t.push(item[i]);
    }
}

void ThreadUnsafeStack::unsafe_pop() {
    if (!stack_t.empty()) {
        int value = stack_t.top();
        std::cout << "thread_id=" << std::this_thread::get_id() << std::endl;
        std::cout << "top value=" << value << std::endl;
        stack_t.pop();
        std::cout << "pop value=" << value << std::endl;
    }
}