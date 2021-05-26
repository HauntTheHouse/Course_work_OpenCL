cmake_minimum_required(VERSION 3.0.0)
set (PROJECT client)
project(${PROJECT})

set(CMAKE_CXX_STANDARD 14)

set(SRCs client.cpp SparseMatrix.cpp SparseMatrix.h)
add_executable(${PROJECT} ${SRCs})

set(TCP_DIR ../TCP)
target_link_libraries(${PROJECT} PRIVATE TCPSocket)
target_include_directories(${PROJECT} PUBLIC ${TCP_DIR})

configure_file(data/pll_post_short_A.txt data/pll_post_short_A.txt COPYONLY)
configure_file(data/sparse_matrix_112.txt data/sparse_matrix_112.txt COPYONLY)
configure_file(data/sparse_matrix_153.txt data/sparse_matrix_153.txt COPYONLY)
configure_file(data/sparse_matrix_132.txt data/sparse_matrix_132.txt COPYONLY)
configure_file(data/sparse_matrix_420.txt data/sparse_matrix_420.txt COPYONLY)