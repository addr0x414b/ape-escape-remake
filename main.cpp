#include <SDL2/SDL.h>
#include <iostream>

int main() {
	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		std::cout << "Failed to initialize the SDL2 library\n";
		return -1;
	}
}
