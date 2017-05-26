cmake_minimum_required(VERSION 3.7)
project(uthreads)

set(CMAKE_CXX_STANDARD 14)

set(SOURCE_FILES main.cpp uthreads.cpp uthreads.h thread.h scheduler.cpp scheduler.h blackbox.cpp blackbox.h debug.h messages.h)
add_executable(uthreads ${SOURCE_FILES})