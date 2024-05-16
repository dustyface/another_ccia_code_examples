#pragma once

#include <iostream>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <list>

struct data_chunk;
extern std::queue<data_chunk> data_queue;

// condition variable
void data_preparation_thread();
void data_processing_thread();
void data_processing_thread2();

// threadsafe_queue.cc
void test_threadsafe_queue();
// packaged_task.cc
void test_packaged_task();
// promise.cc
void test_promise();
