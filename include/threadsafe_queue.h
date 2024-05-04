#include <queue>
#include <memory>
#include <mutex>
#include <condition_variable>

template <typename T>
class ThreadSafeQueue {
private:
    mutable std::mutex mut;
    std::queue<T> data_queue;
    std::condition_variable data_cond;
public:
    ThreadSafeQueue() = default;
    ThreadSafeQueue(const ThreadSafeQueue& other) {
        std::lock_guard<std::mutex> lk(other.mut);
        data_queue = other.data_queue;
    }
    ThreadSafeQueue(T* bg, T* ed) {
        for (auto it = bg; it != ed; it++) {
            data_queue.push(*it);
        }
    }

    void push(T new_value) {
        std::lock_guard<std::mutex> lk(mut);
        data_queue.push(new_value);
        data_cond.notify_one();
    }

    void wait_and_pop(T& value) {
        std::unique_lock<std::mutex> lk(mut);
        data_cond.wait(lk, [&] {
            return !data_queue.empty();
        });
        value = data_queue.front();
        data_queue.pop();
    }

    std::shared_ptr<T> wait_and_pop() {
        std::unique_lock<std::mutex> lk(mut);
        data_cond.wait(lk, [&] {
            return !data_queue.empty();
        });
        std::shared_ptr<T> result(std::make_shared<T>(data_queue.front()));
        data_queue.pop();
        return result;
    }

    bool try_pop(T& value) {
        std::lock_guard<std::mutex> lk(mut);
        if (data_queue.empty())
            return false;
        value = data_queue.front();
        data_queue.pop();
        return true;
    }

    std::shared_ptr<T> try_pop() {
        std::lock_guard<std::mutex> lk(mut);
        if (data_queue.empty())
            return std::shared_ptr<T>();
        std::shared_ptr<T> result(std::make_shared<T>(data_queue.front()));
        data_queue.pop();
        return result;
    }
};

template <>
ThreadSafeQueue<std::string>::ThreadSafeQueue(std::string* bg, std::string* ed) {
    for (auto it = bg; it != ed; ++it) {
        data_queue.push(*it);
    }
}