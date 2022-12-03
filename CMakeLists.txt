project("Concurrent_LinkedListTarea")
cmake_minimum_required(VERSION 3.15)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Threads)

include_directories($(CMAKE_CURRENT_SOURCE_DIR)/src)

add_executable(Concurrent_LinkedListTarea_test src/Concurrent_LinkedListTarea.cpp)
target_link_libraries(Concurrent_LinkedListTarea Threads::Threads)
