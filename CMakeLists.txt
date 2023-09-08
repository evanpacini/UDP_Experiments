cmake_minimum_required(VERSION 3.22)
set(CMAKE_C_STANDARD 23)
project(UDP_Experiments C)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
find_package(Curses REQUIRED)
include_directories(${CURSES_INCLUDE_DIR})

add_executable(ncurses_chat ncurses_chat.c)
target_link_libraries(ncurses_chat ${CURSES_LIBRARIES})
target_link_libraries(ncurses_chat Threads::Threads)
