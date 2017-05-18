cmake_minimum_required(VERSION 3.7)
project(MultiThreadChat_C__)

set(CMAKE_CXX_STANDARD 14)
# set(CMAKE_CXX_FLAGS "-lpthread")
set(CMAKE_CXX_FLAGS "-W -Wall -g -pthread")
set(SOURCE_FILES
        #chatclientTCP.cpp
        chatserverTCP.cpp
        #makefile
        )

add_executable(server ${SOURCE_FILES})