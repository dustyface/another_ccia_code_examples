#pragma once
#include <iostream>
#include <thread>
#include <mutex>
#include <shared_mutex>
#include <stack>
#include "threadsafe_stack.h"

// lock_guard.cc
void test_lock_guard();

// hierarchial_mutex.cc
// hierarchial_mutex 是提供一个层次式mutex实现, 以达到避免死锁目的
class hierarchial_mutex {
    std::mutex internal_mutex;
    // 每线程相关的静态变量; 使用static声明的意义, 在于跨线程保持了值, 但是thread_local又每个线程都有自己的值
    static thread_local unsigned long this_thread_hierarchy_value;
    unsigned long previous_hierarchy_value;
    const unsigned long hierarchy_value;

    void check_for_hierarchy_violation();
    // 当成功获取mutex时, 更新当前线程的hierarchy value
    void update_hierarchy_value();
public:
    explicit hierarchial_mutex(unsigned long value): hierarchy_value(value), previous_hierarchy_value(0) {}
    // mutex class的interface, 包含3个基本操作
    void lock();
    void unlock();
    bool try_lock();
};

void thread_a();
void thread_b(bool);

// deadlock_*.cc
class X {
public:
    // This is **M&M** rule
    // 必须使用mutable修饰std::mutex, 否则在使用std::lock_guard<std::mutex> lk(m)时会报错
    // 原因是，即使是const object, mutex也需要被lock/unlock, 因此需要被mutable修饰
    // hiearchial_mutex 的class中mutex member没有使用 mutable 修饰, 原因是lock_guard<hierarchial_mutex>
    // 使用的是hierarchial_mutex, 而不是std::mutex
    mutable std::mutex m;
    int i = 0;

    // 需要定义default constructor,
    // 因为存在其他constructor, class就不会自动生成默认constructor
    X() = default;
    X(const X& other): i(other.i) {}
    X& operator=(const X&);
    void set(int n);
};
void swap(X& lhs, X& rhs);
void run();

// blow_a_hole.cc, unique_lock.cc
void test_mutex_hole();
void test_unqiue_lock();

// threadunsafe_stack.cc
struct ThreadUnsafeStack {
    std::stack<int> stack_t;
    ThreadUnsafeStack(int* item, size_t size);
    void unsafe_pop();
};

// threadsafe_stack: see threadsafe_stack.h
// threadsafe_stack.cc
void test_threadsafe_stack();

// deadlock_stdlock.cc

namespace stdlock {

class some_big_object {
    std::string* p_str;
public:
    some_big_object(const char*);
    some_big_object(const some_big_object&);
    some_big_object& operator=(const some_big_object&) = delete;
    some_big_object& operator=(some_big_object&& other) noexcept;
    ~some_big_object();
    // friend declaration for operator<<
    friend std::ostream& operator<<(std::ostream& os, const some_big_object& obj);
};

std::ostream& operator<<(std::ostream& os, const some_big_object& obj);
void swap(some_big_object& lhs, some_big_object& rhs);

class XX {
    some_big_object data;
    mutable std::mutex m;
public:
    friend void swap(some_big_object& lhs, some_big_object& rhs);
    XX(const some_big_object&);
    void swap(XX& lhs, XX& rhs);
};

} // namespace stdlock

// shared_lock.cc
// struct dns_entry {
//     std::string ip;
//     std::string host;
// };

// 目前我的clang++ 14.0.0不支持shared_mutex
// class dns_cache {
//     std::map<std::string, dns_entry> entries;
//     mutable std::shared_mutex entry_mutex;
// public:
//     dns_entry find_entry(const std::string&) const;
// };