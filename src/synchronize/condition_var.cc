#include <iostream>
#include "synchronize.h"

struct data_chunk {
    int i;
    std::string s;
};

std::mutex mut;
std::condition_variable  data_cond;
// shared data
std::queue<data_chunk> data_queue;

void process_data(data_chunk& data) {
    std::cout << "process_data: " << data.i << " " << data.s << std::endl;
}

std::istream& operator>>(std::istream& is, data_chunk& data_chunk) {
    // data_chunk并不是shared data, 所以不需要使用mutex在不同线程间保护
    is >> data_chunk.i >> data_chunk.s;
    return is;
}

void get_data(data_chunk& data) {
    std::cout << "pls enter data_chunk: ";
    std::cin >> data;
}

bool is_last_chunk(const data_chunk& data) {
    return data.s == "bye";
}

void data_preparation_thread() {
    while (true) {
        data_chunk data;
        get_data(data);
        {
            std::lock_guard<std::mutex> lk(mut);
            data_queue.push(data);
        }
        data_cond.notify_one();
        if (is_last_chunk(data))
            break;
    }
}

// condition variable 使用方式属于比较简单，但其表达能力其实还是偏弱的
// 1. wait() 方法所封装的机制，是比较复杂的，它带着lock的当前状态，调用predicate，
// 根据它的条件来唤醒或挂起线程;
// 2. 它还可能会spurious awaken的(即不是由于notify方法唤醒，而是由于线程调度
// mutex锁释放就调用了的情况)
// 3. 注意condition varaible的lock，需要使用unique_lock;
void data_processing_thread() {
    while(true) {
        std::unique_lock<std::mutex> lk(mut);
        data_cond.wait(lk, [] {
            return !data_queue.empty();
        });
        data_chunk data = data_queue.front();
        data_queue.pop();
        lk.unlock();
        process_data(data);
        if (is_last_chunk(data))
            break;
    }
}

void data_processing_thread2() {
    data_chunk data;
    while (true) {
        auto const timeout = std::chrono::steady_clock::now() +
            std::chrono::milliseconds(3000);
        std::unique_lock<std::mutex> lk(mut);
        if (data_cond.wait_until(lk, timeout) == std::cv_status::timeout) {
            if (data_queue.empty()) {
                continue;
            }
            data = data_queue.front();
            data_queue.pop();
            process_data(data);
            if (is_last_chunk(data))
                break;
        }
    }
}
