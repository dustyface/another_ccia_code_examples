#include <thread>
#include <mutex>
#include "share_data.h"

////////////////////////////////////////////////////////////////////////////////
// class some_big_object

stdlock::some_big_object::some_big_object(const char* c): p_str(new std::string(c)) {}

stdlock::some_big_object::some_big_object(const some_big_object& other): p_str(new std::string(other.p_str->c_str())) {}

stdlock::some_big_object& stdlock::some_big_object::operator=(some_big_object&& other) noexcept {
    if (this == &other) return *this;
    p_str = other.p_str;
    other.p_str = nullptr;
    return *this;
}

stdlock::some_big_object::~some_big_object() {
    if (p_str != nullptr)
        delete p_str;
}

// operator<< non-member 实现, 需要为friend
std::ostream& stdlock::operator<<(std::ostream& os, const some_big_object& obj) {
    os << *(obj.p_str);
    return os;
}

// operator<< 实现为non-member function, 同时它应该是friend
void stdlock::swap(some_big_object& lhs, some_big_object& rhs) {
    std::cout << "before swap, lhs=" << lhs << ", rhs=" << rhs << std::endl;
    some_big_object temp = std::move(lhs);
    lhs = std::move(rhs);
    rhs = std::move(temp);
    std::cout << "after swap, lhs=" << lhs << ", rhs=" << rhs << std::endl;
}

////////////////////////////////////////////////////////////////////////////////
// class XX
stdlock::XX::XX(const some_big_object& some_detail): data(some_detail) {}

void stdlock::XX::swap(XX& lhs, XX& rhs) {
    if (&lhs == &rhs)
        return;

    // std::scoped & std::unique_lock:
    //
    // 1. std::scoped_lock 是C++17的特性, 它组合lock+lock_guard的能力
    // 不会因调而中断mutex lock, 且RAII style释放资源;
    // std::scoped_lock guard(lhs.m, rhs.m);
    // stdlock::swap(lhs.data, rhs.data);
    //
    // 2. std::unique_lock也可以完成此例，并且它有很大灵活性, 主要包括3点:
    // 延迟lock, 按需lock/unlock, 传递锁
    // std::unique_lock<std::mutex> lk_lhs(lhs.m, std::defer_lock);
    // std::unique_lock<std::mutex> lk_rhs(rhs.m, std::defer_lock);
    // std::lock(lk_lhs, lk_rhs);

    // lock 2 mutex at one uninterceptable operation
    std::lock(lhs.m, rhs.m);
    // std::adopt_lock means accepting the current lock status of the mutex
    std::lock_guard<std::mutex> lk_lhs(lhs.m, std::adopt_lock);
    std::lock_guard<std::mutex> lk_rhs(rhs.m, std::adopt_lock);
    // 如果不加namespace, 单独使用swap, 则会匹配到standard library中的某个参数类型的overloaded swap(但又不能使用std namespace swap)
    stdlock::swap(lhs.data, rhs.data);
}