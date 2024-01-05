add_library(vulkan_context vulkan_context.h vulkan_context.cpp)

find_package(SDL2 CONFIG REQUIRED)
find_package(Vulkan REQUIRED)
target_link_libraries(vulkan_context PRIVATE $<TARGET_NAME_IF_EXISTS:SDL2::SDL2main> $<IF:$<TARGET_EXISTS:SDL2::SDL2>,SDL2::SDL2,SDL2::SDL2-static>)
target_link_libraries(vulkan_context PRIVATE Vulkan::Vulkan)

target_link_libraries(vulkan_context PRIVATE debugger)