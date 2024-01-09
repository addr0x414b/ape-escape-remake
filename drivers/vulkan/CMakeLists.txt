add_library(vulkan_context vulkan_context.h vulkan_context.cpp)

find_package(SDL2 CONFIG REQUIRED)
find_package(Vulkan REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(assimp CONFIG REQUIRED)
target_link_libraries(vulkan_context PRIVATE assimp::assimp)
target_link_libraries(vulkan_context PRIVATE $<TARGET_NAME_IF_EXISTS:SDL2::SDL2main> $<IF:$<TARGET_EXISTS:SDL2::SDL2>,SDL2::SDL2,SDL2::SDL2-static>)
target_link_libraries(vulkan_context PRIVATE Vulkan::Vulkan)
target_link_libraries(ApeEscapeRemake PRIVATE glm::glm)
target_link_libraries(vulkan_context PUBLIC mesh_3d)

target_link_libraries(vulkan_context PRIVATE debugger)
target_link_libraries(vulkan_context PRIVATE stb_image)

set(SHADER_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/shaders")
set(SHADER_BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/shaders")

add_custom_command(
    TARGET vulkan_context POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${SHADER_SOURCE_DIR} ${SHADER_BINARY_DIR}
)

set(TEXTURE_SOURCE_DIR "${CMAKE_SOURCE_DIR}/assets/")
set(TEXTURE_BINARY_DIR "${CMAKE_BINARY_DIR}/assets/")

add_custom_command(TARGET vulkan_context POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${TEXTURE_SOURCE_DIR} ${TEXTURE_BINARY_DIR}
)

set(ASSET_PATH "${CMAKE_BINARY_DIR}/assets")
add_definitions(-DASSET_PATH="${ASSET_PATH}")