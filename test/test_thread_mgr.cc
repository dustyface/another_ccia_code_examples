#include <gtest/gtest.h>
#include "thread_mgr.h"

TEST(test_mutex, test_movable) {
    Movable x(1);
    Movable y = std::move(x);
    // Movable v  = x;     // error: copy constructor is deleted, this would fail to compile

    // 测试编译器的RVO机制, f()的调用并不会引发move constructor的调用
    Movable z = f();
    Movable w = g();
    EXPECT_EQ(*y.p, 1);
    EXPECT_EQ(*z.p, 20);
    EXPECT_EQ(*w.p, 30);
}

TEST(test_mutex, test_dangling_ref) {
    run();
}


