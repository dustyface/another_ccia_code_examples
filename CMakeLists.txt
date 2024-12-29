cmake_minimum_required(VERSION 3.0.0)
project(C++_multithread VERSION 0.1.0 LANGUAGES C CXX)

# Google Test requires at least C++14
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED on)

include(FetchContent)
FetchContent_Declare(
  googletest
#   URL https://github.com/google/googletest/archive/f8d7d77c06936315286eb55f8de22cd23c188571.zip

  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG        f8d7d77c06936315286eb55f8de22cd23c188571 # tag release-1.14.0
)

# below 2 lines ares required for windows platform
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

include_directories(${CMAKE_SOURCE_DIR}/include)
# enabling testing
enable_testing()

add_executable(test_thread_mgr
  test/test_thread_mgr.cc
  src/thread_mgr/init_func.cc
  src/thread_mgr/movable.cc
  src/thread_mgr/dangling_ref.cc
)

add_executable(test_share_data
  test/test_share_data.cc
  src/share_data/hierarchial_mutex.cc
  src/share_data/deadlock_mutualjoin.cc
  src/share_data/deadlock_nestedmutex.cc
  src/share_data/deadlock_stdlock.cc
  src/share_data/lock_guard.cc
  src/share_data/blow_a_hole.cc
  src/share_data/threadunsafe_stack.cc
  src/share_data/threadsafe_stack.cc
)

add_executable(test_synchronize
  test/test_synchronize.cc
  src/synchronize/condition_var.cc
  src/synchronize/threadsafe_queue.cc
  src/synchronize/packaged_task.cc
  src/synchronize/promise.cc
  src/synchronize/time.cc
)

add_executable(test_lockfree
  test/test_lockfree.cc
  src/lockfree/lockfree_stack.cc
)

target_link_libraries(
  test_thread_mgr
  GTest::gtest_main
)
target_link_libraries(
  test_share_data
  GTest::gtest_main
)
target_link_libraries(
  test_synchronize
  GTest::gtest_main
)
target_link_libraries(
  test_lockfree
  GTest::gtest_main
)

# enable CMake's test google test module and enable test discovery
include(GoogleTest)
gtest_discover_tests(test_thread_mgr)
gtest_discover_tests(test_share_data)
gtest_discover_tests(test_synchronize)
gtest_discover_tests(test_lockfree)