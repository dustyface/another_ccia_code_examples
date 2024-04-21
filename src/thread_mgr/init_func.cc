#include <iostream>
#include <thread>
#include "thread_mgr.h"

void init_func() {
    std::cout << "init_func" << std::endl;
}

void BackgroundTask::operator()() const {
    std::cout << "Calling BackgroundTask: " << name << std::endl;
}