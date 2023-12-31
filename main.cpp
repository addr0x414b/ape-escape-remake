#include <SDL2/SDL.h>
#include <iostream>

int main() {
    // Initialize the SDL library
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cout << "Failed to initialize the SDL2 library\n";
        std::cout << SDL_GetError();
        return -1;
    }

    std::cout << "SDL2 library initialized\n";

    // Create a window. Parameters are:
    // 1) Window title
    // 2, 3) X and y starting position
    // 4, 5) Width and height
    // 6) Flags (we are using Vulkan)
    SDL_Window *window =
        SDL_CreateWindow("Ape Escape Remake", SDL_WINDOWPOS_CENTERED,
                         SDL_WINDOWPOS_CENTERED, 800, 600, SDL_WINDOW_VULKAN);

    // If the window failed to be created
    if (!window) {
        std::cout << "Failed to create window\n";
        std::cout << SDL_GetError();
        return -1;
    }

    std::cout << "SDL2 window successfully created\n";

    // Grab the window surface of our previously created window
    SDL_Surface *window_surface = SDL_GetWindowSurface(window);

    // If window surface was failed to be grabbed
    if (!window_surface) {
        std::cout << "Failed to get the surface from the window\n";
        std::cout << SDL_GetError();
        return -1;
    }

    std::cout << "SDL2 window surface successfully created\n";
    std::cout << "SDL2 loop initiated\n";

    // Poll for events
    // If bQuit is true, we stop looping and close the window
    SDL_Event e;
    bool bQuit = false;
    while (!bQuit) {
        while (SDL_PollEvent(&e)) {
            // If event type is quit or escape key is pressed, set bQuit to true
            if (e.type == SDL_QUIT ||
                (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE)) {
                bQuit = true;
            }

            // Update the window surface on each loop
            SDL_UpdateWindowSurface(window);
        }
    }

    std::cout << "Program shut down initiated\n";

    SDL_DestroyWindowSurface(window);
    SDL_DestroyWindow(window);

    std::cout << "End of program reached\n";
}
