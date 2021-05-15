cmake_minimum_required(VERSION 3.0.0)
set(PROJECT TCPSocket)
project(${PROJECT})

add_library(${PROJECT} TCPSocket.cpp)
target_include_directories (${PROJECT} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})