cmake_minimum_required(VERSION 3.0.0)
set (PROJECT server)
project(${PROJECT})

set(CMAKE_CXX_STANDARD 14)

set(SRCs server.cpp)
add_executable(${PROJECT} ${SRCs})

set(TCP_DIR ../TCP)
# add_subdirectory(${TCP_DIR} build)
target_link_libraries(${PROJECT} PRIVATE TCPSocket OpenCL pthread)
target_include_directories(${PROJECT} PUBLIC ${TCP_DIR})

configure_file(kernels/steepestDescent.cl kernels/steepestDescent.cl COPYONLY)
configure_file(kernels/conjugateGradient.cl kernels/conjugateGradient.cl COPYONLY)