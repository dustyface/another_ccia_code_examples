#pragma once

#include <stack>
#include <mutex>
#include <exception>

struct empty_stack: public std::exception {
    const char* what() const noexcept override {
        return "empty stack";
    }
};

// 注意, template class的实现一般是和header声明放在一起,
// 原因是template instantiation必须看到template definition
template <typename T>
class ThreadSafeStack {
private:
    std::stack<T> stack_t;
    mutable std::mutex m;
public:
    ThreadSafeStack() = default;
    ThreadSafeStack(int* begin, int* end);
    ThreadSafeStack(const ThreadSafeStack& other);
    ThreadSafeStack& operator=(const ThreadSafeStack& other) = delete;
    void push(T);
    std::shared_ptr<T> pop();
    void pop(T&);
    bool empty() const;
    size_t size() const;
};

template <typename T>
ThreadSafeStack<T>::ThreadSafeStack(const ThreadSafeStack& other) {
    std::lock_guard<std::mutex> lk(other.m);
    stack_t = other.stack_t;
}

template <typename T>
ThreadSafeStack<T>::ThreadSafeStack(int* begin, int* end) {
    for (auto it = begin; it != end; it++) {
        stack_t.push(*it);
    }
}

template <typename T>
void ThreadSafeStack<T>::push(T value) {
    std::lock_guard<std::mutex> lk(m);
    stack_t.push(value);
}

// 直接返回object，可能会有潜在的copy constructor抛异常的问题;
// 参考:
// https://ptgmedia.pearsoncmg.com/images/020163371x/supplements/Exception_Handling_Article.html
// Herb Sutter的书 Exceptional C++ 有所阐述
// 所以考虑返回 shared_ptr, 因为：
// 1. shared_ptr 的copy constructor是exception safe的;
// 2. shared_ptr 是包含着栈顶元素的指针;
template <typename T>
std::shared_ptr<T> ThreadSafeStack<T>::pop() {
    std::lock_guard<std::mutex> lk(m);
    if (stack_t.empty())
        throw empty_stack();

    const std::shared_ptr<T> result(std::make_shared<T>(stack_t.top()));
    stack_t.pop();
    return result;
}

template <typename T>
void ThreadSafeStack<T>::pop(T& value) {
    std::lock_guard<std::mutex> lk(m);
    if (stack_t.empty())
        throw empty_stack();
    value = stack_t.top();
    stack_t.pop();
}

template <typename T>
bool ThreadSafeStack<T>::empty() const {
    std::lock_guard<std::mutex> lk(m);
    return stack_t.empty();
}

template <typename T>
size_t ThreadSafeStack<T>::size() const {
    std::lock_guard<std::mutex> lk(m);
    return stack_t.size();
}