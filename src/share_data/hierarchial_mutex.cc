#include <exception>
#include "share_data.h"

// initlialy, set the this_thread_hierarchy_value to a large value
// so that the first lock operation will always success
thread_local unsigned long hierarchial_mutex::this_thread_hierarchy_value(ULONG_MAX);

void hierarchial_mutex::check_for_hierarchy_violation() {
    if (this_thread_hierarchy_value <= hierarchy_value) {
        throw std::logic_error("mutex hierarchy violated: current hierarchy value=" +
            std::to_string(this_thread_hierarchy_value) + ", requesting hierarchy value=" +
            std::to_string(hierarchy_value));
    }
}

void hierarchial_mutex::update_hierarchy_value() {
    previous_hierarchy_value = this_thread_hierarchy_value;
    this_thread_hierarchy_value = hierarchy_value;
}

// 可以执行lock, unlock, try_lock操作是有条件的
// lock: 在某个thread执行lock操作, thread_local 变量this_thread_hierarchy_value值相当于一个会动态更新的游标，
// 只允许小于等于这个游标的mutex才能被lock, 由此来避免死锁;
void hierarchial_mutex::lock() {
    check_for_hierarchy_violation();
    internal_mutex.lock();
    update_hierarchy_value();
}

// unlock: 在某一个时刻，只允许和游标值一致的mutex解锁
void hierarchial_mutex::unlock() {
    if (this_thread_hierarchy_value != hierarchy_value) {
        throw std::logic_error("mutex hierarchy violated");
    }
    // 恢复上一次游标的值;
    this_thread_hierarchy_value = previous_hierarchy_value;
    internal_mutex.unlock();
}

bool hierarchial_mutex::try_lock() {
    check_for_hierarchy_violation();
    if (!internal_mutex.try_lock()) {
        return false;
    }
    update_hierarchy_value();
    return true;
}

//////////////////////////////////////////////////////

hierarchial_mutex high_level_mutex(10000);
hierarchial_mutex middle_level_mutex(6000);
hierarchial_mutex low_level_mutex(5000);

int do_lowlevel_stuff() {
    std::cout << "do_lowlevel_stuff" << std::endl;
    return 100;
}

// lock_guard<hierarchial_mutex>
int lowlevel_func() {
    std::lock_guard<hierarchial_mutex> lk(low_level_mutex);
    return do_lowlevel_stuff();
}

void do_highlevel_stuff(int val) {
    std::cout << "do_highlevel_stuff, " << val << std::endl;
}

void highlevel_func() {
    std::lock_guard<hierarchial_mutex> hk(high_level_mutex);
    do_highlevel_stuff(lowlevel_func());
}

void do_other_stuff() {
    highlevel_func();
    std::cout << "do_other_stuff" << std::endl;
}

void thread_a() {
    highlevel_func();
}

void thread_b(bool raise_exception = false) {
    try {
        // 6000 => 10000, error
        std::lock_guard<hierarchial_mutex> mk(middle_level_mutex);
        do_other_stuff();
    } catch(std::logic_error e) {
        std::cout << "e=" << e.what() << std::endl;
        if (raise_exception) {
            throw std::logic_error(e.what());
        }
    }
}