#pragma once
#include <iostream>
#include <thread>

template <typename T>
class lock_free_stack {
private:
    struct node;
    struct counted_node_ptr {
        int external_count;
        node* ptr;
    };
    struct node {
        std::shared_ptr<T> data;
        std::atomic<int> internal_count;
        counted_node_ptr next;
        node(const T& data_): data(std::make_shared<T>(data_)), internal_count(0) {} 
    };
    std::atomic<counted_node_ptr> head;

    void increase_head_count(counted_node_ptr& old_counter, bool pause=false) {
        counted_node_ptr new_counter;
        int loop_count = 0;
        do {
            new_counter = old_counter;
            ++new_counter.external_count;
            loop_count++;
            counted_node_ptr tmp_head = head.load();
        //    if (pause) {
        //         std::cout << "about to pause" << std::endl;
        //         std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        //     }
            std::cout << "do-while loop:"
                << " thread_id=" << std::this_thread::get_id() << ", loop_count=" << loop_count << ", tmp_head.external_count=" << tmp_head.external_count << ", tmp_head.ptr=" << tmp_head.ptr << std::endl;
        } while (!head.compare_exchange_strong(old_counter, new_counter));
        if (pause) {
            std::cout << "about to pause" << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }
        counted_node_ptr tmp_head = head.load();
        std::cout << "increase() after do-while: head.external_count=" << tmp_head.external_count << ", head.ptr=" << tmp_head.ptr
            << ", old_counter.external_count=" << old_counter.external_count << ", old_counter.ptr=" << old_counter.ptr
            << ", new_counter.external_count=" << new_counter.external_count << ", new_counter.ptr=" << new_counter.ptr << std::endl;
        old_counter.external_count = new_counter.external_count;
    }
public:
    ~lock_free_stack() {
        while (pop());
    }
    void push(const T& data) {
        counted_node_ptr new_node;
        new_node.ptr = new node(data);
        new_node.external_count = 1;
        new_node.ptr->next = head.load();
        while (!head.compare_exchange_weak(new_node.ptr->next, new_node));
    }
    std::shared_ptr<T> pop(bool pause = false) {
        std::cout << "pop() start; use_pause=" << pause
            << ", thread_id" << std::this_thread::get_id() << std::endl;
        counted_node_ptr old_head = head.load();
        for (;;) {
            increase_head_count(old_head, pause);
            node* const ptr = old_head.ptr;
            if (!ptr) {
                return std::shared_ptr<T>();
            }
            counted_node_ptr tmp_head = head.load();
            std::cout << "pop(): tmp_head.external_count=" << tmp_head.external_count << ", tmp_head.ptr=" << tmp_head.ptr << std::endl;
            std::cout << "pop(): old_head.external_count=" << old_head.external_count << ", ptr=" << ptr << ", old_head.ptr=" << old_head.ptr << std::endl;
            std::cout << "pop(): ptr->internal_count=" << ptr->internal_count.load() << std::endl;
            if (head.compare_exchange_strong(old_head, ptr->next)) {
                std::shared_ptr<T> res;
                res.swap(ptr->data);
                const int count_increase = old_head.external_count - 2;
                if (ptr->internal_count.fetch_add(count_increase) == -count_increase) {
                    std::cout << "delete ptr 1=" << ptr << ptr->data << std::endl;
                    delete ptr;
                }
                counted_node_ptr tmp_head = head.load();
                std::cout << "first branch; head.external_count=" << tmp_head.external_count << ", head.ptr=" << tmp_head.ptr << std::endl;
                return res;
            } else if (ptr->internal_count.fetch_sub(1) == 1) {
                std::cout << "delete ptr 2=" << ptr << ptr->data <<  std::endl;
                delete ptr;
            }

        }
    }
};

void push_items(int items[], size_t sz);
void pop_items(int size, bool use_pause);
void test_stucture_assign();