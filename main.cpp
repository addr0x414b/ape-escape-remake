#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>
#include <vulkan/vulkan.h>

#include <iostream>
#include <map>
#include <optional>
#include <vector>

// Grab the validation layers from the Vulkan SDK
// These are used to check for errors
const std::vector<const char *> validationLayers = {
    "VK_LAYER_KHRONOS_validation"};

// C++ macro to check if we are in debug mode
// If we are, then we want to enable validation layers
// This is determined by the cmake parameter CMAKE_BUILD_TYPE=Debug or Release
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

// Will contain the queue families
// Queues determine which type of commands we can submit to the device
struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;

    bool isComplete() { return graphicsFamily.has_value(); }
};

QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device) {
    QueueFamilyIndices indices;

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount,
                                             nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount,
                                             queueFamilies.data());

    int i = 0;
    for (const auto &queueFamily : queueFamilies) {
        if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            indices.graphicsFamily = i;
        }
        i++;
    }

    if (indices.isComplete()) {
        return indices;
    } else {
        std::cout << "Failed to find suitable queue family\n";
        return indices;
    }
}

int main() {
    // Print out if we are in debug or release mode
    // Note: currently all debugging is done by calling std::cout. Perhaps this
    // will be changed in the future to use some sort of logging system
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

    // Validation layer information
    // Our debugCallback function is used to print validation information
    // Note the messageSeverity paremeter includes verbose, warning, and error
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

    // Info we need to create the Vulkan instance
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
    // This time we pass the extensions variable to get the actual extensions
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

    // Pass our extension information to the createInfo struct
    createInfo.enabledExtensionCount =
        static_cast<uint32_t>(extensionVector.size());
    createInfo.ppEnabledExtensionNames = extensionVector.data();

    // If we want validation layers, we need to pass the desired layers to the
    // createInfo struct. Otherwise we set the count to 0 since we have none
    if (enableValidationLayers) {
        createInfo.enabledLayerCount =
            static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();
        createInfo.pNext =
            (VkDebugUtilsMessengerCreateInfoEXT *)&createMessengerInfo;
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

    // Following code is to create the debug messenger
    // Only needed if we are using validation layers
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

    // Next, we want to select a physical device
    // This variable will hold the physical device we select
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;

    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
    std::cout << "Found " << deviceCount << " physical devices\n";
    if (deviceCount == 0) {
        std::cout << "Failed to find GPUs with Vulkan support\n";
        return -1;
    }

    // Grab all the available physical devices. Some systems may have several
    // devices we can use, i.e graphics card or integrated graphics
    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

    // Candidates will store all the possible devices
    std::multimap<int, VkPhysicalDevice> candidates;
    // Loop through all the devices and find one that is suitable
    // We want to find a discrete GPU that supports geometry shaders
    // In the event there are several devices, we want to score them and pick
    // the highest rated one
    for (const auto &device : devices) {
        int score = 0;
        VkPhysicalDeviceProperties deviceProperties;
        VkPhysicalDeviceFeatures deviceFeatures;
        vkGetPhysicalDeviceProperties(device, &deviceProperties);
        vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

        // If it's a discrete GPU, we give it a higher score
        if (deviceProperties.deviceType ==
            VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
            score += 1000;
        }

        // Add the max size of textures to the score. Higher = better
        score += deviceProperties.limits.maxImageDimension2D;

        // If the device has no geometry shader, we set the score to 0
        if (!deviceFeatures.geometryShader) {
            score = 0;
        }

        // Insert the device with its score
        candidates.insert(std::make_pair(score, device));
    }

    // If there are no potential candidates
    if (candidates.empty()) {
        std::cout << "Failed to find a suitable GPU\n";
        return -1;
    }

    // Grab the first device (highest score), if none then no suitable
    if (candidates.rbegin()->first > 0) {
        physicalDevice = candidates.rbegin()->second;
    } else {
        std::cout << "Failed to find a suitable GPU\n";
        return -1;
    }

    // A bit of a redundant check, but let's be safe
    if (physicalDevice == VK_NULL_HANDLE) {
        std::cout << "Failed to find suitable GPU\n";
        return -1;
    } else {
        std::cout << "Successfully found a suitable GPU\n";
    }

    // Now let's create the logical device
    VkDevice device;
    // Find the queue families
    QueueFamilyIndices indices = findQueueFamilies(physicalDevice);

    if (!indices.isComplete()) {
        std::cout << "Failed to find suitable queue family\n";
        return -1;
    } else {
        std::cout << "Successfully found suitable queue family\n";
    }

    // Create the queue create info struct
    VkDeviceQueueCreateInfo queueCreateInfo{};
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = indices.graphicsFamily.value();
    queueCreateInfo.queueCount = 1;
    float queuePriority = 1.0f;
    queueCreateInfo.pQueuePriorities = &queuePriority;

    VkPhysicalDeviceFeatures deviceFeatures{};
    // Create the logical device creation info
    VkDeviceCreateInfo createLogicalInfo{};
    createLogicalInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createLogicalInfo.pQueueCreateInfos = &queueCreateInfo;
    createLogicalInfo.queueCreateInfoCount = 1;
    createLogicalInfo.pEnabledFeatures = &deviceFeatures;
    createLogicalInfo.enabledExtensionCount = 0;

    if (enableValidationLayers) {
        createLogicalInfo.enabledLayerCount =
            static_cast<uint32_t>(validationLayers.size());
        createLogicalInfo.ppEnabledLayerNames = validationLayers.data();
    } else {
        createLogicalInfo.enabledLayerCount = 0;
    }

    if (vkCreateDevice(physicalDevice, &createLogicalInfo, nullptr, &device) !=
        VK_SUCCESS) {
        std::cout << "Failed to create logical device\n";
        return -1;
    } else {
        std::cout << "Successfully created logical device\n";
    }

    VkQueue graphicsQueue;
    vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &graphicsQueue);

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
    vkDestroyDevice(device, nullptr);
    vkDestroyInstance(instance, nullptr);
    SDL_DestroyWindowSurface(window);
    SDL_DestroyWindow(window);

    std::cout << "End of program reached\n";
}
