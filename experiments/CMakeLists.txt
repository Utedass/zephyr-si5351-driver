cmake_minimum_required(VERSION 3.16)
project(precision_c_experiment C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_executable(precision
    precision.c
)

# Link the math library
target_link_libraries(precision m)