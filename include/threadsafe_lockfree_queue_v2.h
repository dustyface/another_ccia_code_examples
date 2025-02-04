#pragma once
#include <iostream>
#include <thread>
#include <atomic>

#define PRINT(x) \
    std::cout << ", " << #x << "=" << x

template <typename T>
class lock_free_queue {
private:
    struct node;
    struct counted_node_ptr {
        int external_count;
        node* ptr;
        friend std::ostream& operator<<(std::ostream& os, const counted_node_ptr& ptr);
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
        std::atomic<counted_node_ptr> next;

        node() {
            node_counter new_count;
            // 初始化node的count的值, internal_count=0, external_counters=2
            // 是因为当node被创建加入到queue时, 有2个external_counter，一个是tail, 一个是前一个counted_node_ptr的next
            new_count.internal_count = 0;
            new_count.external_counters = 2;
            count.store(new_count);

            // 注意, 这个next的状态和pop(), push()中的声明的old_next的状态是一致的
            counted_node_ptr new_next;
            new_next.ptr = nullptr;
            new_next.external_count = 0;
            next.store(new_next);
            data.store(nullptr);
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

            counted_node_ptr tmp_counter = counter.load();
            std::cout << "increase_external_count() do-while loop:"
                << " thread_id=" << std::this_thread::get_id();
            PRINT(old_counter.ptr);
            PRINT(old_counter.external_count);
            PRINT(new_counter.ptr);
            PRINT(new_counter.external_count);
            PRINT(tmp_counter.ptr);
            PRINT(tmp_counter.external_count);
            std::cout << std::endl;
        } while (!counter.compare_exchange_strong(old_counter, new_counter,
            std::memory_order_acquire, std::memory_order_relaxed));

        counted_node_ptr tmp_counter = counter.load();
        std::cout << "increase_external_count() ***after*** do-while loop:" << " thread_id=" << std::this_thread::get_id();
        PRINT(tmp_counter.external_count);
        PRINT(tmp_counter.ptr);
        PRINT(new_counter.external_count);
        PRINT(new_counter.ptr);
        PRINT(old_counter.external_count);
        PRINT(old_counter.ptr);
        std::cout << std::endl;

        old_counter.external_count = new_counter.external_count;
    }

    // 处理之后, 对 internal_count 减1，并且在 internal_count 上计算它和 external_counters 的和
    // 来判断是否该删除node
    static void free_external_counter(counted_node_ptr& old_node_ptr) {
        node* const ptr = old_node_ptr.ptr;
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
        } while (!ptr->count.compare_exchange_strong(old_counter, new_counter,
            std::memory_order_acquire, std::memory_order_relaxed));

        node_counter tmp_counter = ptr->count.load();
        std::cout << "free_external_counter(): thread_id=" << std::this_thread::get_id();
        PRINT(tmp_counter.internal_count);
        PRINT(tmp_counter.external_counters);
        std::cout << std::endl;

        if (!new_counter.internal_count && !new_counter.external_counters) {
            delete ptr;
        }
    }

    void set_new_tail(counted_node_ptr& old_tail, const counted_node_ptr& new_tail) {
        node* const current_tail_ptr = old_tail.ptr;

        counted_node_ptr tmp_tail = tail.load();
        std::cout << "set_new_tail(): thread_id=" << std::this_thread::get_id();
        PRINT(tmp_tail.ptr);
        PRINT(tmp_tail.external_count);
        PRINT(old_tail.ptr);
        PRINT(old_tail.external_count);
        PRINT(new_tail.ptr);
        PRINT(new_tail.external_count);
        std::cout << std::endl;

        while (!tail.compare_exchange_weak(old_tail, new_tail) && old_tail.ptr == current_tail_ptr);

        std::cout << "set_new_tail() after loop: thread_id=" << std::this_thread::get_id();
        PRINT(old_tail.ptr);
        PRINT(current_tail_ptr);
        std::cout << std::endl;

        if (old_tail.ptr == current_tail_ptr) {
            free_external_counter(old_tail);
        } else {
            current_tail_ptr->release_ref();
        }
    }

public:
    lock_free_queue() {
        counted_node_ptr new_node;
        new_node.ptr = new node;
        new_node.external_count = 1;
        head.store(new_node);
        tail.store(new_node);
    }

    ~lock_free_queue() {
        while (pop());
    }

    void check_queue() {
        counted_node_ptr tmp_head = head.load();
        std::cout << "check_queue(): thread_id=" << std::this_thread::get_id() << std::endl;
        while (tmp_head.ptr) {
            T* data_ = tmp_head.ptr->data.load();
            std::cout << ", tmp_head.ptr->data=" << *data_ << std::endl;
            tmp_head = tmp_head.ptr->next.load();
        }
    }

    void push(T new_value, bool use_pause=false) {
        std::unique_ptr<T> new_data(new T(new_value));
        counted_node_ptr new_next;
        new_next.ptr = new node;
        new_next.external_count = 1;
        counted_node_ptr old_tail = tail.load();

        std::cout << "push(): thread_id=" << std::this_thread::get_id()
            << ", new_data=" << *new_data << std::endl;

        for (;;) {
            increase_external_count(tail,old_tail);
            T* old_data=nullptr;

            PRINT(old_tail.ptr); std::cout << std::endl;
            T* ptr_data = old_tail.ptr->data.load();
            std::cout << "after increase_external_count(); old_tail.ptr->data=" << ptr_data << ", old_data=" << old_data << std::endl;

            if (old_tail.ptr->data.compare_exchange_strong(
                old_data,new_data.get())) {

                // 初始化old_next为{0}, 是aggregate initialization；它把counted_node_ptr的第一个member为0，其余的为default value, 即external_count为0，ptr为nullptr;
                // 这个和node constructor所创建的next filed的初始化状态是一致的
                // 参见上面的node constructor, 它将next.external_count设为0, next.ptr为nullptr
                counted_node_ptr old_next={0};
                // 1. 当old_tail.ptr->next和old_next相等, 意味着next的状态就是node constructor所创建的默认状态(next.external=0, next.ptr=nullptr)，和old_next完全一致。即意味着，此时还没有加入新的tail node，应该exchange为new_next
                //
                // 2. 当old_taial.ptr->next和old_next不相等，意味着next的状态不是由node constructor所创建的默认状态，next指向着可能由其他线程推入的counted_node_ptr节点(作为tail)
                if (!old_tail.ptr->next.compare_exchange_strong(old_next, new_next)) {
                    delete new_next.ptr;
                    // 此时, 由于old_tail.ptr->next 不等于 old_next, 把old_tail的值赋值给了old_next, ptr->next是新推入的tail, external_count保持着每个线程进入increase_external_count的值;
                    new_next = old_next;
                }
                if (use_pause) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
                    ;
                }
                if (use_pause) {
                    std::cout << "resume from pause; thread_id="
                        << std::this_thread::get_id() << std::endl;
                }
                set_new_tail(old_tail, new_next);
                new_data.release();
                break;
            } else {
                std::cout << "else branch: thread_id=" << std::this_thread::get_id();
                PRINT(old_tail.ptr);
                std::cout << std::endl;
                // tail.ptr->next的默认状态
                counted_node_ptr old_next = {0};
                // 1. 当old_tail.ptr->next和old_next一致时,说明此时是old_tail.ptr->data已经变成push后data的状态了，而next还没有新是默认状态, 需要推入新counte_node_ptr, 因此替换为new_next;
                // 问题: old_next 和 new_next并不是同一个counted_node_ptr，它们是2个不同的object，此种情况下, set_new_tail(old_tail, old_next)会造成链断掉？
                // 2. 当old_tail.ptr->next和old_next不等, 则说明还存在其他线程推入的next counted_node_ptr object, 将old_next替换为old_tail.ptr->next的值后，不执行if内语句, 直接调用set_new_tail, 将tail设定成next所指向的object
                if (old_tail.ptr->next.compare_exchange_strong(old_next,new_next)) {
                    old_next = new_next;
                    new_next.ptr = new node;
                }
                set_new_tail(old_tail, old_next);
                // 执行完后，执行else分支的线程，已经构造了新的tail, 然后去执行for loop, 在去判断tail的ptr->data是否有值否, 决定进入哪个分支;
            }
        }
    }

    std::unique_ptr<T> pop() {
        counted_node_ptr old_head = head.load(std::memory_order_relaxed);
        for (;;) {
            increase_external_count(head, old_head);
            node* const ptr = old_head.ptr;
            if (ptr == tail.load().ptr) {
                // ptr->release_ref();
                return std::unique_ptr<T>();
            }
            counted_node_ptr next=ptr->next.load();
            if (head.compare_exchange_strong(old_head, next)) {
                T* const res = ptr->data.exchange(nullptr);
                free_external_counter(old_head);
                return std::unique_ptr<T>(res);
            }
            ptr->release_ref();
        }
    }
};

void pop_queue_item(int sz);
void push_queue_item(int items[], size_t size, bool use_pause);