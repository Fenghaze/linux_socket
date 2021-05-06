#cmake的版本要求
cmake_minimum_required(VERSION 3.10)
#工程的名称
project(httpServer)

#添加可执行文件
add_executable(server 15-6WebServer.cpp http.cpp)

find_package(Threads)

target_link_libraries(server ${CMAKE_THREAD_LIBS_INIT})