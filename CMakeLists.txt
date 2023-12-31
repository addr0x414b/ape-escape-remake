cmake_minimum_required(VERSION 3.28)

project(ApeEscapeRemake)
add_executable(ApeEscapeRemake main.cpp)

find_package(SDL2 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(Vulkan REQUIRED)

target_link_libraries(ApeEscapeRemake PRIVATE $<TARGET_NAME_IF_EXISTS:SDL2::SDL2main> $<IF:$<TARGET_EXISTS:SDL2::SDL2>,SDL2::SDL2,SDL2::SDL2-static>)
target_link_libraries(ApeEscapeRemake PRIVATE glm::glm)
target_link_libraries(ApeEscapeRemake PRIVATE Vulkan::Vulkan)