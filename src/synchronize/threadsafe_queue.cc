#include <iostream>
#include <thread>
#include "threadsafe_queue.h"

void test_threadsafe_queue() {
    std::string temp[] = {
        "typescript/javascript",
        "python",
        "C/C++",
    };
    ThreadSafeQueue<std::string> queue(begin(temp), end(temp));
    auto process_poped_data = [&queue] {
        std::cout << "process_data, thread_id=" << std::this_thread::get_id() << std::endl;
        while (true) {
            std::shared_ptr<std::string> value = queue.wait_and_pop();
            std::cout << "do some stuff with poped value=" << *value << std::endl;
            if (*value == "Bye")
                break;
        }

    };
    std::function<void()> parepare_data = [&queue] {
        std::cout << "prepare data, thread_id=" << std::this_thread::get_id() << std::endl;
        std::string items[] = {"Rust", "React", "Vue", "Bye"};
        auto bg = begin(items), ed = end(items);
        for (auto it = bg; it != ed; ++it) {
            queue.push(*it);
        }
    };
    std::thread t1(process_poped_data);
    std::thread t2(parepare_data);
    t1.join();
    t2.join();
}

