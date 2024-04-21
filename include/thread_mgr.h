#ifndef MUTEX_H
#define MUTEX_H

#include <string>

// movable.cc
#define MOVE_ONLY(CLS) \
    CLS(CLS&&) noexcept; \
    CLS& operator=(CLS&&) noexcept;

#define DISABLE_COPY_ASSIGN(CLS) \
    CLS(const CLS&) = delete; \
    CLS& operator=(const CLS&) = delete;

class Movable {
  public:
    int *p = nullptr;
    Movable(int i);
    MOVE_ONLY(Movable)
    DISABLE_COPY_ASSIGN(Movable)
};
Movable f();
Movable g();

// init_func.cc
class BackgroundTask {
    std::string name;
public:
    BackgroundTask(const std::string& name_): name(name_) {}
    void operator()() const;
};

// dangling_ref.cc
struct Func;
void run();

#endif