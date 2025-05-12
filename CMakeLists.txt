cmake_minimum_required(VERSION 3.31)
project(networks_labwork)

set(CMAKE_CXX_STANDARD 20)

add_executable(networks_labwork server/server.cpp client/client.cpp)
