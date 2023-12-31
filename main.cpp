#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>
#include <vulkan/vulkan.h>

#include <iostream>
#include <vector>

// Grab the validation layers from the Vulkan SDK
// These are used to check for errors
const std::vector<const char *> validationLayers = {
    "VK_LAYER_KHRONOS_validation"};

// C++ macro to check if we are in debug mode
// If we are, then we want to enable validation layers
#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

// The validation layer debug callback function
static VKAPI_ATTR VkBool32 VKAPI_CALL
debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
              VkDebugUtilsMessageTypeFlagsEXT messageType,
              const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
              void *pUserData) {
    std::cout << "Validation layer: " << pCallbackData->pMessage << std::endl;

    return VK_FALSE;
}

int main() {
    // Print out if we are in debug or release mode
    if (enableValidationLayers) {
        std::cout << "[DEBUG MODE]\n";
    } else {
        std::cout << "[RELEASE MODE]\n";
    }

    std::cout << "Begin initializing SDL2...\n";
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
    // TODO: create an error function we can just call
    if (!window) {
        std::cout << "Failed to create window\n";
        std::cout << SDL_GetError();
        return -1;
    }

    std::cout << "SDL2 window successfully created\n";

    std::cout << "Begin initializing Vulkan...\n";

    // First, check for support of our validation layers.
    // This is important because if there is no support, then we can't use
    // them to debug. If we're not in debug mode, then this doesn't matter.
    // bValidationLayersSupported will stay false if we don't have support.
    // TODO: move this to a function
    bool bValidationLayersSupported = false;
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    for (const char *layerName : validationLayers) {
        bool layerFound = false;

        for (const auto &layerProperties : availableLayers) {
            if (strcmp(layerName, layerProperties.layerName) == 0) {
                layerFound = true;
                break;
            }
        }

        if (layerFound) {
            bValidationLayersSupported = true;
            break;
        }
    }  // End what should be in a function

    // Check if we want validation layers and that they are supported
    // If they are not supported but we want to use them, we exit
    if (enableValidationLayers && !bValidationLayersSupported) {
        std::cout << "Validation layers requested, but not available!\n";
        return -1;
    }

    // Information about the application
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Ape Escape Remake";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "Ape Escape Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_3;

    VkDebugUtilsMessengerCreateInfoEXT createMessengerInfo{};
    if (enableValidationLayers) {
        createMessengerInfo.sType =
            VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        createMessengerInfo.messageSeverity =
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        createMessengerInfo.messageType =
            VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        createMessengerInfo.pfnUserCallback = debugCallback;
        createMessengerInfo.pUserData = nullptr;
    }

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    // We want to grab the extensions from SDL2 so Vulkan knows what additional
    // features we need. We also need to take account of the validation layers
    uint32_t extensionCount = 0;

    // For some reason, we have to pass NULL first to get the count, then we can
    // use that count to malloc the extensions variable. Then finally we can
    // pass extensions to the GetInstanceExtensions function
    SDL_bool extensionResult =
        SDL_Vulkan_GetInstanceExtensions(window, &extensionCount, NULL);

    if (extensionResult == SDL_FALSE) {
        std::cout << "Failed to get the initial Vulkan extension count\n";
        std::cout << SDL_GetError();
        return -1;
    }

    const char **extensions =
        (const char **)malloc(sizeof(char *) * extensionCount);
    extensionResult =
        SDL_Vulkan_GetInstanceExtensions(window, &extensionCount, extensions);

    if (extensionResult == SDL_FALSE) {
        std::cout << "Failed to get the Vulkan extensions\n";
        std::cout << SDL_GetError();
        return -1;
    }

    std::vector<const char *> extensionVector(extensions,
                                              extensions + extensionCount);

    if (enableValidationLayers) {
        extensionVector.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    createInfo.enabledExtensionCount =
        static_cast<uint32_t>(extensionVector.size());
    createInfo.ppEnabledExtensionNames = extensionVector.data();

    // If we want validation layers, we need to the desired layers to the
    // createInfo struct. Otherwise we set the count to 0 since we have none
    if (enableValidationLayers) {
        createInfo.enabledLayerCount =
            static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();
		createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*) &createMessengerInfo;
    } else {
        createInfo.enabledLayerCount = 0;
		createInfo.pNext = nullptr;
    }

    // Create our Vulkan instance
    VkInstance instance;
    VkResult result = vkCreateInstance(&createInfo, nullptr, &instance);
    if (result != VK_SUCCESS) {
        std::cout << "VK instance failed to create\n";
        return -1;
    }

    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
        instance, "vkCreateDebugUtilsMessengerEXT");
    VkDebugUtilsMessengerEXT debugMessenger;
    if (enableValidationLayers) {
        if (func != nullptr) {
            func(instance, &createMessengerInfo, nullptr, &debugMessenger);
            std::cout << "Debug messenger successfully created\n";
        } else {
            std::cout << "Failed to create debug messenger\n";
            return -1;
        }
    }

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

    std::cout << "Program shut down initiated...\n";

    // Below happens when we are shutting down
    // TODO: move this to a function

	// If we are using validation layers, we must destroy the debug messenger on
	// shutdown
    if (enableValidationLayers) {
        auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
            instance, "vkDestroyDebugUtilsMessengerEXT");
        if (func != nullptr) {
            func(instance, debugMessenger, nullptr);
        }
    }
    vkDestroyInstance(instance, nullptr);
    SDL_DestroyWindowSurface(window);
    SDL_DestroyWindow(window);

    std::cout << "End of program reached\n";
}
