cmake_minimum_required(VERSION 3.6)
project(linux_study)

#set(LS_INC_DIRS E:/LIB/linux_std_include/include)
set(CMAKE_CXX_STANDARD 11)

set(SOURCE_FILES
        epoll_client/epoll_client.cpp
        epoll_client/epoll_client.h)

include_directories(E:/LIB/linux_std_include/include)

add_executable(linux_study ${SOURCE_FILES})