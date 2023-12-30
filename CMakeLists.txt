cmake_minimum_required(VERSION 3.28)

project(ApeEscapeRemake)

find_package(SDL2 CONFIG REQUIRED)

add_executable(ApeEscapeRemake main.cpp)

target_link_libraries(ApeEscapeRemake PRIVATE $<TARGET_NAME_IF_EXISTS:SDL2::SDL2main> $<IF:$<TARGET_EXISTS:SDL2::SDL2>,SDL2::SDL2,SDL2::SDL2-static>)
