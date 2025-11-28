cmake_minimum_required(VERSION 3.30)
project(Ass1Files)

set(CMAKE_CXX_STANDARD 26)

add_executable(Ass1Files main.cpp
        cmake-build-debug/AlgoAss.cpp
        cmake-build-debug/AlgoAss.h
        DoctorModule.cpp
        IndexManagers.h)
