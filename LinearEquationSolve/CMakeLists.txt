cmake_minimum_required(VERSION 3.10)
project(LinearSolver CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

# Tìm OpenMP
find_package(OpenMP REQUIRED)

# Build sequential
add_executable(sequential sequential.cpp)
target_compile_options(sequential PRIVATE -O3)

# Build parallel (OpenMP)
add_executable(parallel parallel.cpp)
target_compile_options(parallel PRIVATE -O3)
target_link_libraries(parallel OpenMP::OpenMP_CXX)

message(STATUS "OpenMP found: ${OpenMP_CXX_VERSION}")
