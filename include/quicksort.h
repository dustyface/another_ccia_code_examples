#pragma once
#include <type_traits>
#include <list>
#include <future>

#define DEBUG true

template <typename T>
std::list<T> sequential_quick_sort(std::list<T> input) {
    if (input.empty())
        return input;

    std::list<T> result;
    // 取input的首个元素作为pivot; 只取input.begin()所指的一个元素, insert到result.begin()之前
    // 注意, splice之后，会将被提取的元素从原sequence中删除
    result.splice(result.begin(), input, input.begin());
    const T& pivot = *result.begin();

    if (DEBUG == true)
        std::cout << "pivot=" << pivot << std::endl;

    // 把一个子分区都交给std::partition(处理一些边界情况, e.g. list的元素个数为1 or 2时,都在partition中)
    // return value : divide_point, 是input sequence中第一个大于等于pivot value的元素(不是pivot元素)
    auto divide_point = std::partition(input.begin(), input.end(), [&](const T& t){
        return t < pivot;
    });

    if (DEBUG == true) {
        std::cout << "divide_point=" << *divide_point << std::endl;
        std::cout << "input after parition =";
        for (auto it=input.begin(); it != input.end(); ++it) {
            std::cout << *it << " ";
        }
    }
    std::list<T> lower_part;
    // splice() 4个参数的overload版本, 是将input list的[begin(), end())范围的元素insert到lower_part.end()之前; 注意, divide_point是pivot后的第一个, 不包括此element, 恰好是前半部分;
    // 另外, 在做了此splice之后, input sequence只剩下divide_point之后的部分, input.begin()指向divide_point, 这是生成new_higher很重要的条件;
    lower_part.splice(lower_part.end(), input, input.begin(), divide_point);

    if (DEBUG == true) {
        std::cout << "lower_part=" ;
        for (auto it = lower_part.begin(); it != lower_part.end(); ++it) {
            std::cout << *it << " ";
        }
        std::cout << std::endl;
        std::cout << "input after splice lower_part=" << *(input.begin()) << std::endl;
    }
    // 使用move，避免copy
    auto new_lower(sequential_quick_sort(std::move(lower_part)));
    auto new_higher(sequential_quick_sort(std::move(input)));

    if (DEBUG == true) {
        std::cout << "new_higher=";
        for (auto it = new_higher.begin(); it != new_higher.end(); ++it) {
            std::cout << *it  << " ";
        }
        std::cout << std::endl;
        std::cout << "result1(there should be one element)=";
        for (auto it = result.begin(); it != result.end(); ++it) {
            std::cout << *it << " ";
        }
        std::cout << std::endl;
    }
    // 组织成当前轮的result
    result.splice(result.end(), new_higher);
    result.splice(result.begin(), new_lower);

    if (DEBUG == true) {
        std::cout << "result=";
        for (auto it = result.begin(); it != result.end(); ++it) {
            std::cout << *it << " ";
        }
        std::cout << std::endl;
    }

    return result;
}

template <typename T>
std::list<T> parallel_quicksort(std::list<T> input) {
    std::cout << "parallel quicksort thread id=" << std::this_thread::get_id() << std::endl;
    if (input.empty())
        return input;
    std::list<T> result;
    result.splice(result.begin(), input, input.begin());
    const T& pivot = *result.begin();
    auto divide_point = std::partition(input.begin(), input.end(), [&](const T& t) {
        return t < pivot;
    });
    std::list<T> lower_part;
    lower_part.splice(lower_part.begin(), input, input.begin(), divide_point);
    std::future<std::list<T>> new_lower(std::async(&parallel_quicksort<T>, std::move(lower_part)));
    auto new_higher(parallel_quicksort(std::move(input)));
    result.splice(result.end(), new_higher);
    result.splice(result.begin(), new_lower.get());
    return result;
}

// 这可以是一个通用的工具方法, 用于转换一个function在新的线程执行;
template <typename F, typename Args>
std::future<typename std::result_of<F(Args&&)>::type> spwarn_task(F&& f, Args&& a) {
    typedef typename std::result_of<F(Args&&)>::type result_type;
    // 注意: INVOKE expression 和 Callable signature的区别:
    // 因为std::packaged_task<> template parameter需要的是callable signature,
    // 它的形式是return_type(Args&&)
    // F(Args&&)的形式是Invoke expression(https://en.cppreference.com/w/cpp/utility/functional)
    std::packaged_task<result_type(Args&&)> task(std::move(f));
    std::future<result_type> res(task.get_future());
    std::thread t(std::move(task), std::move(a));
    t.detach();
    return res;
}