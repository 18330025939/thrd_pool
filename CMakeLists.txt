cmake_minimum_required(VERSION 3.10)

project(THRD_POOL)
set(SRC_LIST main.c thread_pool.c)
add_executable(thrd_pool ${SRC_LIST})
target_link_libraries(thrd_pool pthread)
