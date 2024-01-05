#ifndef DISPLAY_SERVER_H
#define DISPLAY_SERVER_H
#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>

#include "core/debugger/debugger.h"
#include "drivers/vulkan/vulkan_context.h"

class DisplayServer {
public:
    void init();
    void cleanup();
    void run();

private:
    Debugger debugger;
    VulkanContext vulkanContext;

    SDL_Window *window;

    void initSDL2();
};
#endif