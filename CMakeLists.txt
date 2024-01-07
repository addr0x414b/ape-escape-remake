cmake_minimum_required(VERSION 3.28)

project(ApeEscapeRemake)
add_executable(ApeEscapeRemake main.cpp)

include_directories(${CMAKE_SOURCE_DIR})
add_subdirectory(core)
add_subdirectory(servers)
add_subdirectory(drivers)
add_subdirectory(thirdparty)

#find_package(SDL2 CONFIG REQUIRED)
#find_package(Vulkan REQUIRED)
#find_package(glm CONFIG REQUIRED)

#target_link_libraries(ApeEscapeRemake PRIVATE $<TARGET_NAME_IF_EXISTS:SDL2::SDL2main> $<IF:$<TARGET_EXISTS:SDL2::SDL2>,SDL2::SDL2,SDL2::SDL2-static>)
#target_link_libraries(ApeEscapeRemake PRIVATE Vulkan::Vulkan)
#target_link_libraries(ApeEscapeRemake PRIVATE glm::glm)

target_link_libraries(ApeEscapeRemake PRIVATE debugger)
#target_link_libraries(ApeEscapeRemake PRIVATE vulkan_context)
target_link_libraries(ApeEscapeRemake PRIVATE display_server)