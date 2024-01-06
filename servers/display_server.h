#ifndef DISPLAY_SERVER_H
#define DISPLAY_SERVER_H
#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>

#include "core/debugger/debugger.h"
#include "drivers/vulkan/vulkan_context.h"

class DisplayServer {
   public:
    // Initialize SDL2 and Vulkan
    void init();

    // Destroy all SDL2 and Vulkan objects and quit SDL2
    void cleanup();

    // Display server loop
    void run();

   private:
    Debugger debugger;
    VulkanContext vulkanContext;

    SDL_Window *window;

    // Initialize SDL2 and create a window
    void initSDL2();
};
#endif