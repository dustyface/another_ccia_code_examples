#include <thread>
#include "threadsafe_lockfree_stack.h"

lock_free_stack<int> stack;

void push_items(int items[], size_t size) {
    for (int i = 0; i < size; i++) {
        stack.push(items[i]);
    }
}

void pop_items(int size) {
    for (int i = 0; i < size; i++) {
        std::shared_ptr<int> res = stack.pop();
        if (res)
            std::cout << "thread_id=" << std::this_thread::get_id() << std::endl;
            std::cout << "pop item=" << *res << std::endl;
    }
}