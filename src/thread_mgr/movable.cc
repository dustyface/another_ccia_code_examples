#include <iostream>
#include "thread_mgr.h"


Movable::Movable(int i): p(new int(i)) {
	std::cout << "Movable constructor start, i=" << i << "," << &i << std::endl;
}

Movable::Movable(Movable&& f_) noexcept {
	std::cout << "Movable move_constructor start" << std::endl;
	if (this != &f_) { // guard self-assignment
		int* pp = p;
		p = f_.p;
		delete pp;
		f_.p = nullptr;
	}
}
Movable& Movable::operator=(Movable&& f_) noexcept {
	std::cout << "Movable move_assignment_opeator start" << std::endl;
	if (this != &f_) {
		int *pp = p;
		p = f_.p;
		delete pp;
		f_.p = nullptr;
	}
	return *this;
}

// f(), g(), 测试RVO机制:
// 即在move onley, disable copy & assignment operator的class情况中,
// 从函数返回对象时, 会引发compiler的RVO机制，并不会发生2次调用move constructor

Movable f() {
	// m对象，只能被move，返回Movable intance m
	// 可以返回lvalue，会引发针对return的优化RVO
	// **由于RVO, move constructor并没有得到调用**
	Movable m(20);
	return m;
}

Movable g() {
	// 可以返回rvalue
	return Movable(30);
}
