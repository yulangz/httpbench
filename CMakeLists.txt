# cmake_minimum_required(VERSION 3.17)
project(httpbench C)

set(CMAKE_C_STANDARD 11)

add_executable(httpbench httpbench.c parse_url.c build_header.c utils.c circular_queue.c)