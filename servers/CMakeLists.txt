add_library(display_server display_server.h display_server.cpp)

find_package(SDL2 CONFIG REQUIRED)
target_link_libraries(display_server PRIVATE $<TARGET_NAME_IF_EXISTS:SDL2::SDL2main> $<IF:$<TARGET_EXISTS:SDL2::SDL2>,SDL2::SDL2,SDL2::SDL2-static>)

target_link_libraries(display_server PRIVATE vulkan_context)
target_link_libraries(display_server PRIVATE debugger)
