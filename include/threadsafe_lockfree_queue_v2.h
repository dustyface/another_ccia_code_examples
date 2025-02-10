#pragma once
#include <iostream>
#include <thread>
#include <atomic>

#define PRINT_THREADID(label) \
    std::cout << __LINE__ << " " << __FUNCTION__ << "(): " \
    << #label << ", thread_id=" << std::this_thread::get_id() << std::endl;
#define PRINT(x) \
    std::cout << __LINE__ << " " << __FUNCTION__ << "(): " \
    << #x << "=" << (x) << std::endl

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
        std::atomic<counted_node_ptr> next;

        node() {
            node_counter new_count;
            // 初始化node的count的值, internal_count=0, external_counters=2
            // 是因为当node被创建加入到queue时, 有2个external_counter，一个是tail, 一个是前一个counted_node_ptr的next
            new_count.internal_count = 0;
            new_count.external_counters = 2;
            count.store(new_count);

            // 注意, 这个next的状态和pop(), push()中的声明的old_next的状态是一致的
            // 用于标记next节点还不存在用户数据的状态
            counted_node_ptr new_next = {0};
            // 踩坑: new_next如下设置则compare_exchange_strong和old_next{0}会失败;
            // new_next.ptr = nullptr;
            // new_next.external_count = 0;
            next.store(new_next);
            data.store(nullptr);
        }

        void release_ref() {
            // node_counter old_counter = count.load(std::memory_order_relaxed);
            node_counter old_counter = count.load();
            node_counter new_counter;
            do {
                new_counter = old_counter;
                --new_counter.internal_count;
                PRINT(old_counter.internal_count);
                PRINT(old_counter.external_counters);
                PRINT(new_counter.internal_count);
            } while (!count.compare_exchange_strong(old_counter, new_counter));
            // } while (!count.compare_exchange_strong(old_counter, new_counter,
                // std::memory_order_acquire, std::memory_order_relaxed));

            PRINT_THREADID("after loop");
            PRINT(new_counter.internal_count);
            PRINT(new_counter.external_counters);
            PRINT(new_counter.internal_count == new_counter.external_counters);

            if (!new_counter.internal_count && !new_counter.external_counters) {
                delete this;
            }
        }
    };

    // 处理之前，对tail或head external_counter进行加1
    // 当counter不等于old_counter时, 说明有其他线程已经改变了old_counter值, 会重新更新old_counter的值，重新计算
    // 当old_counter值被更新，会影响到外面调用侧的实参(old_tail or old_head)
    static void increase_external_count(std::atomic<counted_node_ptr>& counter,
        counted_node_ptr& old_counter) {
        PRINT_THREADID("check old_counter");
        PRINT(old_counter.ptr);
        PRINT(old_counter.external_count);

        counted_node_ptr new_counter;
        do {
            new_counter = old_counter;
            ++new_counter.external_count;
       } while (!counter.compare_exchange_strong(old_counter, new_counter));

        old_counter.external_count = new_counter.external_count;
        // check update result
        counted_node_ptr tmp_counter = counter.load();
        PRINT_THREADID("after do-while loop: (tmp_counter is the tail or head)");
        PRINT(tmp_counter.external_count);
        PRINT(tmp_counter.ptr);
        PRINT(old_counter.external_count);
        PRINT(old_counter.ptr);
        PRINT(new_counter.ptr);
        PRINT(new_counter.external_count);
    }

    /**
     * 释放外部计数器。
     * @param old_node_ptr node 实例的引用, e.g. old_tail or old_head
     */
    static void free_external_counter(counted_node_ptr& old_node_ptr) {
        node* const ptr = old_node_ptr.ptr;
        // count_increase 代表除了当前线程以及初始值之外, 其他在处理线程的个数
        int count_increase  = old_node_ptr.external_count - 2;
        // node_counter old_counter = ptr->count.load(std::memory_order_relaxed);
        node_counter old_counter = ptr->count.load();
        node_counter new_counter;
        do {
            // PRINT_THREADID("in do-while loop");
            // PRINT(old_counter.internal_count);
            // PRINT(old_counter.external_counters);
            // PRINT(old_node_ptr.external_count);
            // PRINT(count_increase);

            new_counter = old_counter;
            // node's count external_counters 减1
            --new_counter.external_counters;
            // node's count internal_count 减去count_increase
            new_counter.internal_count += count_increase;
        } while (!ptr->count.compare_exchange_strong(old_counter, new_counter));
        // } while (!ptr->count.compare_exchange_strong(old_counter, new_counter,
            // std::memory_order_acquire, std::memory_order_relaxed));

        node_counter tmp_counter = ptr->count.load();
        PRINT_THREADID("after loop");
        PRINT(tmp_counter.internal_count);
        PRINT(tmp_counter.external_counters);
        if (!new_counter.internal_count && !new_counter.external_counters) {
            delete ptr;
        }
    }

    void set_new_tail(counted_node_ptr& old_tail, const counted_node_ptr& new_tail) {
        node* const current_tail_ptr = old_tail.ptr;

        counted_node_ptr tmp_tail = tail.load();
        PRINT_THREADID("before loop:");
        PRINT(new_tail.ptr);
        PRINT(old_tail.ptr);
        PRINT(tail.load().ptr);

        while (!tail.compare_exchange_weak(old_tail, new_tail) && old_tail.ptr == current_tail_ptr);

        PRINT_THREADID("after loop");
        PRINT(old_tail.ptr == current_tail_ptr);

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
        tail.store(head.load());
    }

    ~lock_free_queue() {
        while (pop());
    }

    void print_queue() {
        counted_node_ptr current_node = head.load();
        PRINT_THREADID("print_queue(): ");
        PRINT(head.load().ptr);
        PRINT(head.load().external_count);
        unsigned int ele_count = 0;
        counted_node_ptr fake_next = {0};

        while (current_node.ptr && current_node.ptr->data.load()) {
        // while (current_node.ptr && !current_node.ptr->next.compare_exchange_strong(fake_next, fake_next)) {
            std::cout << "queue element NO.=" << ele_count++ << std::endl;
            PRINT(current_node.external_count);
            PRINT(current_node.ptr);
            T* data_ = current_node.ptr->data.load();
            PRINT(*data_);
            current_node = current_node.ptr->next.load();
        }
    }

    void push(T new_value, bool use_pause=false) {
        std::unique_ptr<T> new_data(new T(new_value));
        counted_node_ptr new_next;
        new_next.ptr = new node;
        new_next.external_count = 1;
        PRINT_THREADID("check new_next.ptr");
        PRINT(new_next.ptr);
        counted_node_ptr old_tail = tail.load();

        PRINT_THREADID("pushing: "); PRINT(*new_data);
        for (;;) {
            increase_external_count(tail,old_tail);
            T* old_data=nullptr;

            T* current_data = old_tail.ptr->data.load();
            PRINT_THREADID("check if data is nullptr:");
            PRINT(current_data);
            PRINT(old_data);
            PRINT(old_tail.ptr);

            if (old_tail.ptr->data.compare_exchange_strong(
                old_data,new_data.get())) {

                // 初始化old_next为{0}, 是aggregate initialization；它把counted_node_ptr的第一个member为0，其余的为default value, 即external_count为0，ptr为nullptr;
                // 这个和node constructor所创建的next filed的初始化状态是一致的
                // 参见上面的node constructor, 它将next.external_count设为0, next.ptr为nullptr
                counted_node_ptr old_next={0};

                PRINT_THREADID("check data & next");
                T* tmp_data = old_tail.ptr->data.load();
                counted_node_ptr tmp_next = old_tail.ptr->next.load();
                PRINT(*tmp_data);
                PRINT(tmp_next.ptr);
                PRINT(tmp_next.external_count);

                if (use_pause) {
                    PRINT_THREADID("about to pause");
                    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
                    PRINT_THREADID("resume from pause");
                }
                // 1. 当old_tail.ptr->next和old_next相等, 意味着next的状态就是node constructor所创建的默认状态(next.external=0, next.ptr=nullptr,标记为没有用户数据的状态)，和old_next完全一致。即意味着，此时还没有加入新的tail node，应该exchange为new_next(即为push 新的counted_node_ptr节点)
               // 2. 当old_taial.ptr->next和old_next不相等(进入if branch)，next表示已经由其他线程推入的counted_node_ptr节点
                if (!old_tail.ptr->next.compare_exchange_strong(old_next, new_next)) {
                    PRINT_THREADID("old_tail.ptr->next is not equal to old_next");
                    // 当此时next已经是别的counted_node_ptr, 则此时为new_next.ptr分配的内存应该释放, 并将new_next替换成dummy node old_next{0}; 注意, 此时old_next被copy replace 为old_tail.ptr->next的值
                    delete new_next.ptr;
                    new_next = old_next;
                }

                set_new_tail(old_tail, new_next);
                new_data.release();
                break;
            } else {
                PRINT_THREADID("else branch:");
                PRINT(old_tail.ptr);
                // tail.ptr->next的默认状态
                counted_node_ptr old_next = {0};
                // 1. 当old_tail.ptr->next和old_next一致时,说明此时是old_tail.ptr->data已经变成push后data的状态了，而next还没有update 默认状态, 需要推入新counte_node_ptr, 因此替换为new_next;
                // 2. 当old_tail.ptr->next和old_next不等, 则说明还存在其他线程推入的next counted_node_ptr object, 将old_next替换为old_tail.ptr->next的值后，不执行if内语句, 直接调用set_new_tail, 将tail设定成next所指向的object
                if (old_tail.ptr->next.compare_exchange_strong(old_next,new_next)) {
                    PRINT_THREADID("else branch: old_tail.ptr->next is replaced by new_next");
                    PRINT(old_next.ptr);
                    PRINT(new_next.ptr);
                    old_next = new_next;
                    new_next.ptr = new node;
                    PRINT(old_next.ptr);
                    PRINT(new_next.ptr);
                }
                set_new_tail(old_tail, old_next);
                // 执行完后， 继续执行for loop, 在去判断tail的ptr->data是否有值否, 决定进入哪个分支;
            }
        }
    }

    std::unique_ptr<T> pop() {
        PRINT_THREADID("pop start:");
        // counted_node_ptr old_head = head.load(std::memory_order_relaxed);
        counted_node_ptr old_head = head.load();
        for (;;) {
            increase_external_count(head, old_head);
            node* const ptr = old_head.ptr;
            if (ptr == tail.load().ptr) {
                ptr->release_ref();
                return std::unique_ptr<T>();
            }
            counted_node_ptr next=ptr->next.load();

            if (head.compare_exchange_strong(old_head, next)) {
                PRINT("swap out data and return");
                T* const res = ptr->data.exchange(nullptr);
                free_external_counter(old_head);
                return std::unique_ptr<T>(res);
            }
            PRINT_THREADID("about to release_ref")
            ptr->release_ref();
        }
    }
};

void pop_queue_item(int sz);
void push_queue_item(int items[], size_t size, bool use_pause);
void print_queue();