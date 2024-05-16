#include <gtest/gtest.h>
#include "synchronize.h"
#include "quicksort.h"

TEST(test_synchronize, test_condition_var) {
    std::thread t1(data_preparation_thread);
    std::thread t2(data_processing_thread);
    t1.join();
    t2.join();
}

TEST(test_synchronize, test_condition_var2) {
    std::thread t1([&] {
        std::cout << "you have 5s to input element" << std::endl;
        data_preparation_thread();
    });
    std::thread t2(data_processing_thread2);
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

TEST(test_synchronize, test_seq_quicksort) {
    std::list<int> lst{5, 7, 3, 4, 1, 9, 2, 8, 10, 6};
    std::cout << "lst.size()=" <<  lst.size() << std::endl;
    std::list<int> result = sequential_quick_sort(lst);
    std::cout << "result.size()=" << result.size() << std::endl;
    std::cout << "result=";
    for (auto it=result.begin(); it != result.end(); ++it) {
        std::cout << *it << " ";
    }
    std::cout << std::endl;
}

TEST(test_synchronize, test_parallel_quicksort) {
    std::list<int> list{5, 7, 3, 4, 1, 9, 2, 8, 10, 6};
    std::cout << "lst.size()=" <<  list.size() << std::endl;
    std::list<int> result = parallel_quicksort(list);
    std::cout << "result.size()=" << result.size() << std::endl;
    std::cout << "result=";
    for (auto it=result.begin(); it != result.end(); ++it) {
        std::cout << *it << " ";
    }
    std::cout << std::endl;
}