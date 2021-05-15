cmake_minimum_required(VERSION 3.0.0)

set (PROJECT Course_Work_Linear_Equation)
project(${PROJECT})

add_subdirectory(TCP build)
add_subdirectory(client)
add_subdirectory(server)