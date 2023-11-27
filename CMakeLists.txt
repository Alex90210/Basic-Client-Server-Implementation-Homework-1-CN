cmake_minimum_required(VERSION 3.26)
project(homework1)

set(CMAKE_CXX_STANDARD 17)

add_executable(homework1 main.cpp
        client.c
        server.c)
