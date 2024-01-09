add_subdirectory(3d)

add_library(scene scene.h scene.cpp)
target_link_libraries(scene PRIVATE mesh_3d)
target_link_libraries(scene PRIVATE display_server)
target_link_libraries(scene PRIVATE debugger)

set(ASSET_PATH "${CMAKE_BINARY_DIR}/assets")
add_definitions(-DASSET_PATH="${ASSET_PATH}")