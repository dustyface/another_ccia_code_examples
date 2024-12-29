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

    void increase_head_count(counted_node_ptr& old_counter) {
        counted_node_ptr new_counter;
        do {
            new_counter = old_counter;
            ++new_counter.external_count;
            std::cout << "head=" << &head << ", old_counter=" << &old_counter << std::endl;
        } while (!head.compare_exchange_strong(old_counter, new_counter));
        std::cout << "old_counter=" << &old_counter << ", new_counter" << &new_counter << std::endl;
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
        // std::cout << "push(): new_node.ptr=" << new_node.ptr << ", head=" << head << std::endl;
        while (!head.compare_exchange_weak(new_node.ptr->next, new_node));
    }
    std::shared_ptr<T> pop() {
        counted_node_ptr old_head = head.load();
        for (;;) {
            increase_head_count(old_head);
            node* const ptr = old_head.ptr;
            if (!ptr) {
                return std::shared_ptr<T>();
            }
            if (head.compare_exchange_strong(old_head, ptr->next)) {
                std::cout << "about to delete ptr" << std::endl;
                std::shared_ptr<T> res;
                res.swap(ptr->data);
                const int count_increase = old_head.external_count - 2;
                if (ptr->internal_count.fetch_add(count_increase) == -count_increase) {
                    delete ptr;
                }
                return res;
            } else if (ptr->internal_count.fetch_sub(1) == 1) {
                std::cout << "about to fetch_sub internal count, " << ptr->internal_count << std::endl;
                delete ptr;
            }

        }
    }
};

void push_items(int items[], size_t sz);
void pop_items(int size);