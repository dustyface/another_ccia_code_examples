#include <gtest/gtest.h>
#include "share_data.h"

TEST(test_share_data, test_hierarchial_mutex) {
    // hierarchial_mutex  10000 => 6000, ok
    std::thread t1(thread_a);
    t1.join();
    // hierarchial_mutex 5000 => 1000, error
    std::thread t2(thread_b, false);
    t2.join();
    // below code can't handle the exception thrown
    // ASSERT_THROW({
    //     std::thread t3(thread_b, true);
    // }, std::logic_error);
}

// 故意制造嵌套的mutex调用，导致死锁
TEST(test_share_data, test_deadlock_nestedmutex) {
    std::cout << "test main thread start" << std::endl;
    X a;
    X b;
    a.set(10);
    b.set(20);

    // This will cause deadlock due to nested mutex invocation in 2 threads
    // 另外注意,  thread initial funciton swap的参数是reference
    // 需要注意，在定义thread时，传参需要std::ref, 否则编译不通过
    std::thread t(swap, std::ref(a), std::ref(b));
    std::thread d(swap, std::ref(b), std::ref(a));
    t.join();
    d.join();
    std::cout << "test main thread end" << std::endl;
}

TEST(test_share_data, test_stdlock) {
    stdlock::XX a("hello");
    stdlock::XX b("std::lock");
    std::thread t1(&stdlock::XX::swap, &a, std::ref(a), std::ref(b));
    std::thread t2(&stdlock::XX::swap, &b, std::ref(b), std::ref(a));
    t1.join();
    t2.join();
}

// 故意制造了互相join的情况,导致死锁
TEST(test_share_data, test_deadlock_mutualjoin) {
    run();
}

TEST(test_share_data, test_lock_guard) {
    test_lock_guard();
}

// 连续做N次test_mutex_hole, 会发现输出不是稳定的同一个值
TEST(test_share_data, test_mutex_hole) {
    for (int i = 0; i < 20; i++)
        test_mutex_hole();
}

TEST(test_share_data, test_unqiue_lock) {
    for (int i = 0; i < 10; i++)
        test_unqiue_lock();
}

// 输出是不稳定的, 2个thread调度的次序是不确定的
TEST(test_share_data, test_threadunsafe_stack) {
    int a[] = {1, 2, 3, 4, 5 };
    ThreadUnsafeStack stack(a, sizeof(a)/sizeof(a[0]));
    std::thread t1([&stack]{
        stack.unsafe_pop();
    });
    // thread接受bind形式的用法, 第一个参数是关联的this对象
    std::thread t2(&ThreadUnsafeStack::unsafe_pop, &stack);
    t1.join();
    t2.join();
    std::cout << "stack's length=" << stack.stack_t.size()
        << ", stack's top=" << stack.stack_t.top() << std::endl;
}

TEST(test_share_data, test_threadsafe_stack) {
    test_threadsafe_stack();
}


