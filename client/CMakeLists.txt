cmake_minimum_required(VERSION 3.0.0)
set (PROJECT client)
project(${PROJECT})

set(CMAKE_CXX_STANDARD 14)

set(SRCs client.cpp SparseMatrix.cpp SparseMatrix.h)
add_executable(${PROJECT} ${SRCs})

set(TCP_DIR ../TCP)
# add_subdirectory(${TCP_DIR} build)
target_link_libraries(${PROJECT} PRIVATE TCPSocket)
target_include_directories(${PROJECT} PUBLIC ${TCP_DIR})

configure_file(data/pll_post_short_A.txt data/pll_post_short_A.txt COPYONLY)
configure_file(data/bcsstk05.txt data/bcsstk05.txt COPYONLY)
#configure_file(data/bcsstk01.txt data/bcsstk01.txt COPYONLY)
configure_file(data/sparse_matrix.txt data/sparse_matrix.txt COPYONLY)