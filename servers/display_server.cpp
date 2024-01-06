#include "display_server.h"

// Initialize SDL2 and create a window
void DisplayServer::initSDL2() {
    debugger.consoleMessage("\nBegin initializing SDL2...", false);

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        debugger.consoleMessage("Failed to initialize SDL2 video!", false);
        debugger.consoleMessage(SDL_GetError(), true);
    } else {
        debugger.consoleMessage("Successfully initialized SDL2 video", false);
    }

    window =
        SDL_CreateWindow("Ape Escape Remake", SDL_WINDOWPOS_CENTERED,
                         SDL_WINDOWPOS_CENTERED, 800, 600, SDL_WINDOW_VULKAN);

    if (!window) {
        debugger.consoleMessage("Failed to create SDL2 window!", false);
        debugger.consoleMessage(SDL_GetError(), true);
    } else {
        debugger.consoleMessage("Successfully created SDL2 window", false);
    }

    // Pass the window to the Vulkan context for the surface
    vulkanContext.setWindow(window);

    debugger.consoleMessage("Successfully initialized SDL2\n", false);
}

// Destroy all SDL2 and Vulkan objects and quit SDL2
void DisplayServer::cleanup() {
    debugger.consoleMessage("\nBegin cleaning up display server...", false);
    vulkanContext.cleanup();
    SDL_DestroyWindowSurface(window);
    debugger.consoleMessage("Destroyed SDL2 window surface", false);
    SDL_DestroyWindow(window);
    debugger.consoleMessage("Destroyed SDL2 window", false);
    SDL_Quit();
    debugger.consoleMessage("Quit SDL2", false);
    debugger.consoleMessage("\nSuccessfully cleaned up display server", false);
}

// Initialize SDL2 and Vulkan
void DisplayServer::init() {
    initSDL2();
    vulkanContext.initVulkan();
}

// Display server loop
void DisplayServer::run() {
    debugger.consoleMessage("\nBegin running display server...", false);
    SDL_Event e;
    bool bQuit = false;
    while (!bQuit) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT ||
                e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE) {
                bQuit = true;
            }
            // Vulkan context handles drawing to the surface
            vulkanContext.drawFrame();
        }
    }
}