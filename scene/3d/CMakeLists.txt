add_library(mesh_3d mesh_3d.h mesh_3d.cpp)

find_package(assimp CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(Vulkan REQUIRED)

target_link_libraries(mesh_3d PRIVATE debugger)
target_link_libraries(mesh_3d PRIVATE assimp::assimp)
target_link_libraries(mesh_3d PRIVATE stb_image)
target_link_libraries(mesh_3d PRIVATE vulkan_context)
target_link_libraries(mesh_3d PUBLIC glm::glm)
target_link_libraries(mesh_3d PRIVATE Vulkan::Vulkan)
