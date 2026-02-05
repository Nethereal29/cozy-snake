cmake_minimum_required(VERSION 3.16)
project(cozy_snake_puzzle LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(SDL2 REQUIRED)

add_executable(cozy-snake
  src/main.cpp
)

target_include_directories(cozy-snake PRIVATE ${SDL2_INCLUDE_DIRS})
target_link_libraries(cozy-snake PRIVATE ${SDL2_LIBRARIES})
