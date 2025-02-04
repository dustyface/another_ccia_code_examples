#pragma once
#include <iostream>
#include <atomic>

template <typename T>
class lock_free_queue {
private:
    struct node;
    struct counted_node_ptr {
        int external_count;
        node* ptr;
    };
    std::atomic<counted_node_ptr> head;
    std::atomic<counted_node_ptr> tail;
    struct node_counter {
        unsigned int internal_count:30;
        unsigned int external_counters:2;
    };
    struct node {
        std::atomic<T*> data;
        std::atomic<node_counter> count;
        counted_node_ptr next;

        node() {
            node_counter new_count;
            // 初始化node的count的值, internal_count=0, external_counters=2
            // 是因为当node被创建加入到queue时, 有2个external_counter，一个是tail, 一个是前一个counted_node_ptr的next
            new_count.internal_count = 0;
            new_count.external_counters = 2;
            count.store(new_count);

            next.ptr = nullptr;
            next.external_count = 0;
        }
        // 处理完相应的工作, 对node的internal_count减1, 判断是否删除node
        void release_ref() {
            node_counter old_counter = count.load(std::memory_order_relaxed);
            node_counter new_counter;
            do {
                new_counter = old_counter;
                --new_counter.internal_count;
            } while (!count.compare_exchange_strong(old_counter, new_counter,
                std::memory_order_acquire, std::memory_order_relaxed));
            if (!new_counter.internal_count && !new_counter.external_counters) {
                delete this;
            }
        }
    };

    // 处理之前，对external_counter进行加1;
    static void increase_external_count(std::atomic<counted_node_ptr>& counter, 
        counted_node_ptr& old_counter) {
        counted_node_ptr new_counter;
        do {
            new_counter = old_counter;
            ++new_counter.external_count;
        } while (!count.compare_exchange_strong(old_counter, new_counter,
            std::memory_order_acquire, std::memory_order_relaxed));
        old_counter.external_count = new_counter.external_count;
    }

    // 处理之后, 对 internal_count 减1，并且在 internal_count 上计算它和 external_counters 的和
    // 来判断是否该删除node
    static void free_external_counter(counted_node_ptr& old_node_ptr) {
        const node* ptr = old_node_ptr.ptr;
        // 计算除了当前线程以及初始值之外, 其他在处理线程的个数
        int count_increase  = old_node_ptr.external_count - 2;
        node_counter old_counter = ptr->count.load(std::memory_order_relaxed);
        node_counter new_counter;
        do {
            new_counter = old_counter;
            // 问题?
            // 没有增加count里的external_counters的位置，只有初始化为2, 且free时就减少, 并判断当它为0时，是删除ptr的条件之一?
            --new_counter.external_counters;
            new_counter.internal_count += count_increase;
        } while (!count.compare_exchange_strong(old_counter, new_counter, 
            std::memory_order_acquire, std::memory_order_relaxed));
        if (!new_counter.internal_count && !new_counter.external_counters) {
            delete ptr;
        }
    }

public:
    void push(T new_value) {
        std::unique_ptr<T> new_data(new T(new_value));
        counted_node_ptr new_next;
        new_next.ptr = new node;
        new_next.external_count = 1;
        counted_node_ptr old_tail = tail.load();
        for (;;) {
            increase_external_count(tail, old_tail);
            T* old_data = nullptr;
            // 注释1的位置, 虽然没有使用mutex, 但其实会引发lock, 形成busy-wait; 
            // 因为当一个线程执行到此处时, 另一个线程old_tail->ptr.data 和 old_data 不会相等, 去执行next loop, 形成busy-wait
            if (old_tail.ptr->data.compare_exchange_strong(old_data, new_data.get())) {  // 1
                old_tail.ptr->next = new_next;
                old_tail = tail.exchange(new_next);
                free_external_counter(old_tail);
                new_data.release();
                break;
            }
            old_tail.ptr->release_ref();
        }
    }

    std::unique_ptr<T> pop() {
        counted_node_ptr old_head = head.load(std::memory_order_relaxed);
        for (;;) {
            increase_external_count(head, old_head);
            node* const ptr = old_head.ptr;
            if (ptr == tail.load().ptr) {
                ptr->release_ref();
                return std::unique_ptr<T>();
            }
            if (head.compare_exchange_strong(old_head, ptr->next)) {
                T* const res = ptr->data.exchange(nullptr);
                free_external_counter(old_head);
                return std::unique_ptr<T>(res);
            }
            ptr->release_ref();
        }
    }
};