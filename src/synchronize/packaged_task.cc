#include <iostream>
#include <mutex>
#include <future>
#include <deque>
#include <thread>

std::mutex queue_mut;
std::deque<std::packaged_task<std::string()>> tasks;
bool is_quit_message = false;

bool gui_shutdown_message_received() {
    return is_quit_message == true;
}

void gui_thread() {
    std::cout << "gui_thread_id=" << std::this_thread::get_id() << std::endl;
    while(!gui_shutdown_message_received()) {
        std::packaged_task<std::string()> task;
        {
            std::lock_guard<std::mutex> lk(queue_mut);
            if (tasks.empty())
                continue;
            task = std::move(tasks.front());
            tasks.pop_front();
        }
        // 注意:
        // 在gui_thread线程一侧, 或者在post_task_for_gui_thread线程一侧，都可以
        // 通过get_future()访问future object, 通过get()访问return value
        // 但是, 如果在某一侧使用过get_future, 再次访问已经被move了的object, 则会引发异常
        std::future<std::string> f = task.get_future();
        task();
        std::cout << "future result=" << f.get() << std::endl;
    }
}

// push task thread
template <typename Func>
void post_task_for_gui_thread(Func f) {
    std::packaged_task<std::string()> task(f);
    // 注意:
    // future是movable only的, 如果在push thread这一侧率先通过get_future()
    // 取得future对象时, 在gui_thread线程中, 再通过task.get_future()时，会造成异常;
    // std::future<std::string> res = task.get_future();
    {
        std::lock_guard<std::mutex> lk(queue_mut);
        tasks.push_back(std::move(task));
    }
    // future.get()会阻塞线程, 当在gui_thread线程run task之后, 恢复调度此线程
    // 才会输出值;
    // 注意, 如果这个future get()方法在上面的inner scope内, 则是在没有释放mutex的情况下
    // 尝试获取值, 此时gui_thread无法申请到mutex, 无法run task, 整体上造成死锁;
    // std::cout << "value=" << res.get() << std::endl;
}

// this is message action class
struct MsgAction {
    std::string msg;
    MsgAction(std::string s): msg(s) {}
    std::string operator()() {
        std::cout << "run task: thread_id=" << std::this_thread::get_id() << std::endl;
        std::cout << "msg=" << msg << std::endl;
        return msg;
    }
};

std::string last_message() {
    std::cout << "last_message: thread_id=" << std::this_thread::get_id() << std::endl;
    is_quit_message = true;
    return "last msg";
}

// 检查run task的thread id是否和gui_thread的thread id一致,
// 结论: 是一致的, 意味着future并不开启另一个线程去执行任务
void test_packaged_task() {
    std::thread t1([&]{
        std::cout << "thread t1 id=" << std::this_thread::get_id() << std::endl;
        MsgAction m[] = {
            MsgAction("hello"), MsgAction("C++"), MsgAction("concurrency")
        };
        for (auto it = std::begin(m); it != std::end(m); ++it) {
           post_task_for_gui_thread(*it);
        }
        post_task_for_gui_thread(last_message);
    });
    std::thread t2(gui_thread);
    t1.join();
    t2.join();
}