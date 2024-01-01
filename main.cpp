#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>
#include <vulkan/vulkan.h>

#include <algorithm>
#include <fstream>
#include <iostream>
#include <map>
#include <optional>
#include <set>
#include <vector>

// Grab the validation layers from the Vulkan SDK
// These are used to check for errors
const std::vector<const char *> validationLayers = {
    "VK_LAYER_KHRONOS_validation"};

// Required device extensions
const std::vector<const char *> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME};

// C++ macro to check if we are in debug mode
// If we are, then we want to enable validation layers
// This is determined by the cmake parameter CMAKE_BUILD_TYPE=Debug or Release
#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

// The validation layer debug callback function
// All this does is print out the callback data such as errors or warnings
static VKAPI_ATTR VkBool32 VKAPI_CALL
debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
              VkDebugUtilsMessageTypeFlagsEXT messageType,
              const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
              void *pUserData) {
    std::cout << "Validation layer: " << pCallbackData->pMessage << std::endl;

    return VK_FALSE;
}

// Contains details about the swap chain capabilities
// This is used in the QueueFamilyIndices struct. We get swap chain info
// in the same function that we get the queue families
struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

// Will contain the queue families
// Queues determine which type of commands we can submit to the device
// We need a graphics family for graphics processing and a present family for
// actually presenting to the Vulkan surface
// TODO: Make this "device support" struct or something like that, since it
// has multiple purposes
struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;
    std::vector<VkExtensionProperties> deviceExtensions;
    SwapChainSupportDetails swapChainSupport;

    // If the device has a graphics family and a present family queue, then
    // there will be a value for those parameters. So we check here to make
    // sure that we have both
    bool isComplete() {
        return graphicsFamily.has_value() && presentFamily.has_value();
    }
};

// This function searches for all the various queue families that the device has
// Also searches for device extensions and gets swap chain details
QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device,
                                     VkSurfaceKHR surface) {
    // Will store the queue families
    // This is based on a struct we created at the top of this file
    QueueFamilyIndices indices;

    // Grab the number of queue families
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount,
                                             nullptr);

    // Store the queue families in a vector
    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount,
                                             queueFamilies.data());

    // Loop through all of the queue families and check if they have the
    // graphics support and present support. If so, set them
    int i = 0;
    for (const auto &queueFamily : queueFamilies) {
        if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            indices.graphicsFamily = i;
        }

        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface,
                                             &presentSupport);

        if (presentSupport) {
            indices.presentFamily = i;
        }
        i++;
    }

    // Grab the extensions the device supports
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount,
                                         nullptr);

    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount,
                                         availableExtensions.data());

    std::set<std::string> requiredExtensions(deviceExtensions.begin(),
                                             deviceExtensions.end());

    // Go through the availabe extensions, and remove from the required ones
    // If required is empty, then we know we found all the required extensions
    for (const auto &extension : availableExtensions) {
        requiredExtensions.erase(extension.extensionName);
    }

    // Set our available extensions
    indices.deviceExtensions = availableExtensions;

    // Start getting the swap chain details
    SwapChainSupportDetails details;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface,
                                              &details.capabilities);

    // Find number of formats we have available
    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount,
                                         nullptr);

    // If we have at least one format, store the format in the swapchain struct
    if (formatCount != 0) {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount,
                                             details.formats.data());
    }

    // Find the number of present modes
    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface,
                                              &presentModeCount, nullptr);

    // If we have at least one present mode, store it in the swapchain struct
    if (presentModeCount != 0) {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(
            device, surface, &presentModeCount, details.presentModes.data());
    }

    bool swapChainAdequate = false;

    // If we found all the required extensions, then set swapchain adequate to
    // true only if we have at least one format and one present mode
    if (requiredExtensions.empty()) {
        swapChainAdequate =
            !details.formats.empty() && !details.presentModes.empty();
    }

    // Return the queue families indices if they are complete
    // and we have all the required extensions and swap chain is adequate
    if (indices.isComplete() && requiredExtensions.empty() &&
        swapChainAdequate) {
        indices.swapChainSupport = details;
        return indices;
    } else {
        // Error message isn't fully accurate, could be variety of reasons
        std::cout << "Failed to find suitable queue family\n";
        return indices;
    }
}

// Grab the swap chain surface format
VkSurfaceFormatKHR chooseSwapSurfaceFormat(
    const std::vector<VkSurfaceFormatKHR> &availableFormats) {
    for (const auto &availableFormat : availableFormats) {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB &&
            availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return availableFormat;
        }
    }

    return availableFormats[0];
}

// Grab the swap chain present mode
VkPresentModeKHR chooseSwapPresentMode(
    const std::vector<VkPresentModeKHR> &availablePresentModes) {
    for (const auto &availablePresentMode : availablePresentModes) {
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return availablePresentMode;
        }
    }

    return VK_PRESENT_MODE_FIFO_KHR;
}

// Get the swap chain extent (resolution)
VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities,
                            SDL_Window *window) {
    if (capabilities.currentExtent.width !=
        std::numeric_limits<uint32_t>::max()) {
        return capabilities.currentExtent;
    } else {
        int width, height;
        SDL_Vulkan_GetDrawableSize(window, &width, &height);

        VkExtent2D actualExtent = {static_cast<uint32_t>(width),
                                   static_cast<uint32_t>(height)};

        actualExtent.width =
            std::clamp(actualExtent.width, capabilities.minImageExtent.width,
                       capabilities.maxImageExtent.width);
        actualExtent.height =
            std::clamp(actualExtent.height, capabilities.minImageExtent.height,
                       capabilities.maxImageExtent.height);

        return actualExtent;
    }
}

// This function reads in the shader files
static std::vector<char> readFile(const std::string &filename) {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    std::cout << "Attempting to open shader file...\n";
    if (!file.is_open()) {
        std::cout << "Failed to open shader file\n";
        return std::vector<char>();
    }

    size_t fileSize = (size_t)file.tellg();
    std::vector<char> buffer(fileSize);

    file.seekg(0);
    file.read(buffer.data(), fileSize);

    file.close();
    std::cout << "File successfully open and read and closed\n";
    return buffer;
}

// This function wraps up the shader code so we can send it to the graphics
// pipeline
VkShaderModule createShaderModule(VkDevice device,
                                  const std::vector<char> &code) {
    VkShaderModuleCreateInfo createShaderInfo{};
    createShaderInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createShaderInfo.codeSize = code.size();
    createShaderInfo.pCode = reinterpret_cast<const uint32_t *>(code.data());
    VkShaderModule shaderModule;
    if (vkCreateShaderModule(device, &createShaderInfo, nullptr,
                             &shaderModule) != VK_SUCCESS) {
        std::cout << "Failed to create shader module\n";
        return shaderModule;
    } else {
        std::cout << "Shader module successfully created\n";
        return shaderModule;
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
    std::cout << "Begin creating SDL2 window...\n";

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

    if (enableValidationLayers) {
        std::cout << "Validation layers enabled. Checking for support...\n";
    }

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
    } else {
        std::cout << "Validation layers successfully enabled\n";
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

    std::cout << "Beging getting required extensions for Vulkan...\n";

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
    } else {
        std::cout << "Successfully got initial extension count for Vulkan\n";
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
    } else {
        std::cout << "Successfully got the Vulkan extensions\n";
    }

    std::vector<const char *> extensionVector(extensions,
                                              extensions + extensionCount);

    if (enableValidationLayers) {
        std::cout << "Validation layers are enabled. Adding debug utils to "
                     "extensions\n";
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

    std::cout << "Creating Vulkan instance...\n";

    // Create our Vulkan instance
    VkInstance instance;
    VkResult result = vkCreateInstance(&createInfo, nullptr, &instance);
    if (result != VK_SUCCESS) {
        std::cout << "Vulkan instance failed to create\n";
        return -1;
    } else {
        std::cout << "Vulkan instance successfully created\n";
    }

    // Following code is to create the debug messenger
    // Only needed if we are using validation layers
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
        instance, "vkCreateDebugUtilsMessengerEXT");
    VkDebugUtilsMessengerEXT debugMessenger;
    if (enableValidationLayers) {
        if (func != nullptr) {
            func(instance, &createMessengerInfo, nullptr, &debugMessenger);
            std::cout << "Vulkan debug messenger successfully created\n";
        } else {
            std::cout << "Failed to create Vulkan debug messenger\n";
            return -1;
        }
    }

    std::cout << "Begin creating Vulkan window surface...\n";

    // Create the window surface
    VkSurfaceKHR surface;
    SDL_bool surfaceResult =
        SDL_Vulkan_CreateSurface(window, instance, &surface);

    if (surface == VK_FALSE) {
        std::cout << "Failed to create window surface\n";
        std::cout << SDL_GetError();
        return -1;
    } else {
        std::cout << "Vulkan window surface successfully created\n";
    }

    std::cout << "Begin selecting physical device...\n";

    // Next, we want to select a physical device
    // This variable will hold the physical device we select
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;

    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
    if (deviceCount == 0) {
        std::cout << "Failed to find GPUs with Vulkan support\n";
        return -1;
    } else {
        std::cout << "Found " << deviceCount
                  << " compatible physical devices\n";
    }

    std::cout << "Begin scoring physical devices...\n";

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
        std::cout << "Found best suitable device\n";
    } else {
        std::cout << "Failed to find a suitable GPU\n";
        return -1;
    }

    if (physicalDevice == VK_NULL_HANDLE) {
        std::cout << "Failed to find suitable GPU\n";
        return -1;
    }

    std::cout << "Begin creating logical device...\n";

    // Now let's create the logical device
    VkDevice device;
    // Find the queue families
    QueueFamilyIndices indices = findQueueFamilies(physicalDevice, surface);

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = {indices.graphicsFamily.value(),
                                              indices.presentFamily.value()};

    if (!indices.isComplete()) {
        std::cout << "Failed to find suitable queue families\n";
        return -1;
    } else {
        std::cout << "Successfully found suitable queue families\n";
    }

    // Create the queue create info struct
    // VkDeviceQueueCreateInfo queueCreateInfo{};
    // queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    // queueCreateInfo.queueFamilyIndex = indices.graphicsFamily.value();
    // queueCreateInfo.queueCount = 1;
    float queuePriority = 1.0f;
    // queueCreateInfo.pQueuePriorities = &queuePriority;

    for (uint32_t queueFamily : uniqueQueueFamilies) {
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    VkPhysicalDeviceFeatures deviceFeatures{};
    // Create the logical device creation info
    VkDeviceCreateInfo createLogicalInfo{};
    createLogicalInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createLogicalInfo.pQueueCreateInfos = queueCreateInfos.data();
    createLogicalInfo.queueCreateInfoCount =
        static_cast<uint32_t>(queueCreateInfos.size());
    createLogicalInfo.pEnabledFeatures = &deviceFeatures;
    createLogicalInfo.enabledExtensionCount =
        static_cast<uint32_t>(deviceExtensions.size());
    createLogicalInfo.ppEnabledExtensionNames = deviceExtensions.data();

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
    VkQueue presentQueue;
    vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &graphicsQueue);
    vkGetDeviceQueue(device, indices.presentFamily.value(), 0, &presentQueue);

    std::cout << "Begin creating swapchain...\n";

    // Grab swap chain details
    VkSurfaceFormatKHR surfaceFormat =
        chooseSwapSurfaceFormat(indices.swapChainSupport.formats);
    VkPresentModeKHR presentMode =
        chooseSwapPresentMode(indices.swapChainSupport.presentModes);
    VkExtent2D extent =
        chooseSwapExtent(indices.swapChainSupport.capabilities, window);

    // Add an extra to the minimum image count just to be safe
    uint32_t imageCount =
        indices.swapChainSupport.capabilities.minImageCount + 1;

    // If for some reason our image count is larger than our max, set it to max
    if (indices.swapChainSupport.capabilities.maxImageCount > 0 &&
        imageCount > indices.swapChainSupport.capabilities.maxImageCount) {
        imageCount = indices.swapChainSupport.capabilities.maxImageCount;
    }

    // Swap chain creation struct
    VkSwapchainCreateInfoKHR createSwapInfo{};
    createSwapInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createSwapInfo.surface = surface;
    createSwapInfo.minImageCount = imageCount;
    createSwapInfo.imageFormat = surfaceFormat.format;
    createSwapInfo.imageColorSpace = surfaceFormat.colorSpace;
    createSwapInfo.imageExtent = extent;
    createSwapInfo.imageArrayLayers = 1;
    createSwapInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    uint32_t queueFamilyIndices[] = {indices.graphicsFamily.value(),
                                     indices.presentFamily.value()};

    // If for some reason the graphics family is different than the present
    // family
    if (indices.graphicsFamily != indices.presentFamily) {
        createSwapInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createSwapInfo.queueFamilyIndexCount = 2;
        createSwapInfo.pQueueFamilyIndices = queueFamilyIndices;
    } else {
        createSwapInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createSwapInfo.queueFamilyIndexCount = 0;
        createSwapInfo.pQueueFamilyIndices = nullptr;
    }

    createSwapInfo.preTransform =
        indices.swapChainSupport.capabilities.currentTransform;
    createSwapInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createSwapInfo.presentMode = presentMode;
    createSwapInfo.clipped = VK_TRUE;
    createSwapInfo.oldSwapchain = VK_NULL_HANDLE;

    VkSwapchainKHR swapChain;

    // Create the swap chain
    if (vkCreateSwapchainKHR(device, &createSwapInfo, nullptr, &swapChain) !=
        VK_SUCCESS) {
        std::cout << "Failed to create swapchain\n";
        return -1;
    } else {
        std::cout << "Successfully created swapchain\n";
    }

    std::cout << "Begin creating swap chain image views...\n";

    // Start making the swap chain image views
    std::vector<VkImage> swapChainImages;
    vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
    swapChainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(device, swapChain, &imageCount,
                            swapChainImages.data());

    VkFormat swapChainImageFormat;
    VkExtent2D swapChainExtent;

    swapChainImageFormat = surfaceFormat.format;
    swapChainExtent = extent;

    std::vector<VkImageView> swapChainImageViews;
    swapChainImageViews.resize(swapChainImages.size());

    // For the number of images, create the structs and image views
    for (size_t i = 0; i < swapChainImages.size(); i++) {
        VkImageViewCreateInfo createImageInfo{};
        createImageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createImageInfo.image = swapChainImages[i];
        createImageInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createImageInfo.format = swapChainImageFormat;
        createImageInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        createImageInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createImageInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createImageInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        createImageInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        createImageInfo.subresourceRange.baseMipLevel = 0;
        createImageInfo.subresourceRange.levelCount = 1;
        createImageInfo.subresourceRange.baseArrayLayer = 0;
        createImageInfo.subresourceRange.layerCount = 1;

        if (vkCreateImageView(device, &createImageInfo, nullptr,
                              &swapChainImageViews[i]) != VK_SUCCESS) {
            std::cout << "Failed to create image views\n";
            return -1;
        } else {
            std::cout << "Successfully created image view\n";
        }
    }

    std::cout << "Successfully created all image views for swap chain\n";

    std::cout << "Begin creating renderpass...\n";

    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = swapChainImageFormat;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout - VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;

    VkRenderPass renderPass;
    VkPipelineLayout pipelineLayout;

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;

    if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
        std::cout << "Failed to create renderpass\n";
        return -1;
    }

    std::cout << "Successfully created renderpass\n";

    std::cout << "Begin creating graphics pipeline...\n";

    auto vertShaderCode = readFile("vert.spv");
    auto fragShaderCode = readFile("frag.spv");

    VkShaderModule vertShaderModule =
        createShaderModule(device, vertShaderCode);
    VkShaderModule fragShaderModule =
        createShaderModule(device, fragShaderCode);

    VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
    vertShaderStageInfo.sType =
        VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vertShaderModule;
    vertShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
    fragShaderStageInfo.sType =
        VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragShaderModule;
    fragShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo,
                                                      fragShaderStageInfo};

    std::vector<VkDynamicState> dynamicStates = {
        VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR
    };

    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicState.pDynamicStates = dynamicStates.data();

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 0;
    vertexInputInfo.pVertexBindingDescriptions = nullptr;
    vertexInputInfo.vertexAttributeDescriptionCount = 0;
    vertexInputInfo.pVertexAttributeDescriptions = nullptr;

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float) swapChainExtent.width;
    viewport.height = (float) swapChainExtent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = swapChainExtent;

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;
    rasterizer.depthBiasConstantFactor = 0.0f;
    rasterizer.depthBiasClamp = 0.0f;
    rasterizer.depthBiasSlopeFactor = 0.0f;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampling.minSampleShading = 1.0f;
    multisampling.pSampleMask = nullptr;
    multisampling.alphaToCoverageEnable = VK_FALSE;
    multisampling.alphaToOneEnable = VK_FALSE;

    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    colorBlending.blendConstants[0] = 0.0f;
    colorBlending.blendConstants[1] = 0.0f;
    colorBlending.blendConstants[2] = 0.0f;
    colorBlending.blendConstants[3] = 0.0f;

    //VkPipelineLayout pipelineLayout;
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 0;
    pipelineLayoutInfo.pSetLayouts = nullptr;
    pipelineLayoutInfo.pushConstantRangeCount = 0;
    pipelineLayoutInfo.pPushConstantRanges = nullptr;

    if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
        std::cout << "Failed to create pipeline layout\n";
        return -1;
    } else {
        std::cout << "Successfully created pipeline layout\n";
    }

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState =  &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = nullptr;
    pipelineInfo.pColorBlendState =  &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = pipelineLayout;
    pipelineInfo.renderPass = renderPass;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineInfo.basePipelineIndex = -1;

    VkPipeline graphicsPipeline;
    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS) {
        std::cout << "Failed to create graphics pipeline\n";
        return -1;
    }

    std::cout << "Successfully created graphics pipeline\n";

    std::cout << "Vulkan successfully initialized\n";

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

    vkDestroyPipeline(device, graphicsPipeline, nullptr);
    vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
    vkDestroyRenderPass(device, renderPass, nullptr);
    vkDestroyShaderModule(device, fragShaderModule, nullptr);
    vkDestroyShaderModule(device, vertShaderModule, nullptr);
    for (auto imageView : swapChainImageViews) {
        vkDestroyImageView(device, imageView, nullptr);
    }

    vkDestroySwapchainKHR(device, swapChain, nullptr);

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
    vkDestroySurfaceKHR(instance, surface, nullptr);
    vkDestroyInstance(instance, nullptr);
    SDL_DestroyWindowSurface(window);
    SDL_DestroyWindow(window);

    std::cout << "End of program reached\n";
}
