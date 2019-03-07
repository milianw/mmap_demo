cmake_minimum_required(VERSION 3.0)
project(mmap_demo)

find_package(Threads)
find_package(Qt5 COMPONENTS Core)
set(CMAKE_AUTOMOC TRUE)
set(CMAKE_CXX_STANDARD 17)

add_executable(mmap_demo mmap_demo.cpp)
target_link_libraries(mmap_demo Threads::Threads Qt5::Core)
