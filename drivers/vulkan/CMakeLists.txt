add_library(vulkan_context vulkan_context.h vulkan_context.cpp)

find_package(SDL2 CONFIG REQUIRED)
find_package(Vulkan REQUIRED)
target_link_libraries(vulkan_context PRIVATE $<TARGET_NAME_IF_EXISTS:SDL2::SDL2main> $<IF:$<TARGET_EXISTS:SDL2::SDL2>,SDL2::SDL2,SDL2::SDL2-static>)
target_link_libraries(vulkan_context PRIVATE Vulkan::Vulkan)

target_link_libraries(vulkan_context PRIVATE debugger)

#file(COPY shaders DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
set(SHADER_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/shaders")
set(SHADER_BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/shaders")

add_custom_command(
    TARGET vulkan_context POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${SHADER_SOURCE_DIR} ${SHADER_BINARY_DIR}
    COMMENT "Copying shaders to build directory"
)