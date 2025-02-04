#include "threadsafe_lockfree_queue_v2.h"

lock_free_queue<int> queue;

void push_queue_item(int items[], size_t size, bool use_pause) {
    for (int i = 0; i < size; i++) {
        queue.push(items[i], use_pause);
    }
}

void pop_queue_item(int sz) {
    for (int i=0; i<sz; i++) {
        std::cout << "pop_queue_item: i=" << i << std::endl;
        std::unique_ptr<int> res = queue.pop();
        if (res)
            std::cout << "poped item=" << *res << std::endl;
    }
}