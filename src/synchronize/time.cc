#include <chrono>

using namespace std::chrono_literals;

auto one_day = 10.5h;

auto min = 2.5min;
std::chrono::duration<long double, std::ratio<60, 1>> min2(2.5);