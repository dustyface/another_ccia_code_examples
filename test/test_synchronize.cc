#include <gtest/gtest.h>
#include "synchronize.h"

TEST(test_synchronize, test_condition_var) {
    std::thread t1(data_preparation_thread);
    std::thread t2(data_processing_thread);
    t1.join();
    t2.join();
}

TEST(test_synchronize, test_threadsafe_queue) {
    test_threadsafe_queue();
}

TEST(test_synchronize, test_packged_task) {
    test_packaged_task();
}

TEST(test_synchronize, test_promise) {
    test_promise();
}