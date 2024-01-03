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

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;

    bool isComplete() {
        return graphicsFamily.has_value() && presentFamily.has_value();
    }
};

// Stores capabilities, formats, and present modes of a swapchain
struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

// Print to the console a message, or throw an error
void consoleMessage(const char *message, bool error) {
    if (error) {
        throw std::runtime_error(message);
    } else {
        std::cout << message << std::endl;
    }
}

// Validation layers we wish to enable in debug mode
const std::vector<const char *> validationLayers = {
    "VK_LAYER_KHRONOS_validation"};

const std::vector<const char *> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME};

// Check if the validation layers we want are supported by the system
bool checkValidationLayerSupport() {
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    // Grab all the available validation layers
    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    // Loop our requested layers and available layers and if we have a match,
    // we simply return true indicating that we have support
    for (const char *layerName : validationLayers) {
        bool layerFound = false;

        for (const auto &layerProperties : availableLayers) {
            if (strcmp(layerName, layerProperties.layerName) == 0) {
                layerFound = true;
                break;
            }
        }

        if (!layerFound) {
            return false;
        }
    }
    return true;
}

// Returns the required Vulkan extensions for SDL2
std::vector<const char *> getRequiredExtensions(SDL_Window *window) {
    uint32_t extensionCount = 0;

    // Will return true on success
    SDL_bool extensionResult =
        SDL_Vulkan_GetInstanceExtensions(window, &extensionCount, NULL);

    if (extensionResult == SDL_FALSE) {
        consoleMessage("Failed to get required instance extension count!",
                       false);
        consoleMessage(SDL_GetError(), true);
    } else {
        consoleMessage("Successfully got required instance extension count",
                       false);
    }

    // Allocate memory for the extension names and store them in the array
    const char **sdl2Extensions =
        (const char **)malloc(sizeof(char *) * extensionCount);
    extensionResult = SDL_Vulkan_GetInstanceExtensions(window, &extensionCount,
                                                       sdl2Extensions);

    if (extensionResult == SDL_FALSE) {
        consoleMessage("Failed to get required instance extensions!", false);
        consoleMessage(SDL_GetError(), true);
    } else {
        consoleMessage("Successfully got required instance extensions", false);
    }

    // Create a vector with the SDL2 extensions up to the extension count
    // This is so we can add the debug extension if we are in debug mode
    std::vector<const char *> extensions(sdl2Extensions,
                                         sdl2Extensions + extensionCount);

    if (enableValidationLayers) {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    return extensions;
}

static VKAPI_ATTR VkBool32 VKAPI_CALL
debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
              VkDebugUtilsMessageTypeFlagsEXT messageType,
              const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
              void *pUserData) {
    std::cout << "Validation layer: " << pCallbackData->pMessage << "\n\n";
    return VK_FALSE;
}

VkResult createDebugUtilsMessengerEXT(
    VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo,
    const VkAllocationCallbacks *pAllocator,
    VkDebugUtilsMessengerEXT *pDebugMessenger) {
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
        instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    } else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

// Grabs the queue families of the GPU (graphics and present)
QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device,
                                     VkSurfaceKHR surface) {

    // This is what we'll return
    QueueFamilyIndices indices;

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount,
                                             nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount,
                                             queueFamilies.data());

    // Once we have all the queue families, loop over them
    int i = 0;
    for (const auto &queueFamily : queueFamilies) {
        // If we match graphics queue, set the index
        if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            indices.graphicsFamily = i;
        }

        // This will be true if we find a queue family that supports presenting
        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface,
                                             &presentSupport);

        if (presentSupport) {
            indices.presentFamily = i;
        }

        // If both present and graphics are seat, we are complete
        if (indices.isComplete()) {
            break;
        }

        i++;
    }

    return indices;
}

// Makes sure we have the required extensions for the GPU
bool checkDeviceExtensionSupport(VkPhysicalDevice device) {
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount,
                                         nullptr);

    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount,
                                         availableExtensions.data());

    // We set the deviceExtensions at the top of this file,
    // thus these are the required extensions we want
    std::set<std::string> requiredExtensions(deviceExtensions.begin(),
                                             deviceExtensions.end());

    // Loop through the available extension and if we find a match, remove
    // from the required extension vector
    for (const auto &extension : availableExtensions) {
        requiredExtensions.erase(extension.extensionName);
    }

    // If we have found all the required extensions, return true
    return requiredExtensions.empty();
}

// Makes sure we have support for a swapchain
// Function wil return the capabilities, formats, and present modes
SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device,
                                              VkSurfaceKHR surface) {
    SwapChainSupportDetails details;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface,
                                              &details.capabilities);

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount,
                                         nullptr);

    if (formatCount != 0) {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount,
                                             details.formats.data());
    }

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface,
                                              &presentModeCount, nullptr);

    if (presentModeCount != 0) {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(
            device, surface, &presentModeCount, details.presentModes.data());
    }
    return details;
}

// Checks if the GPU is suitable for our needs
bool isDeviceSuitable(VkPhysicalDevice device, VkSurfaceKHR surface) {
    QueueFamilyIndices indices = findQueueFamilies(device, surface);

    // Make sure we have all the required extensions
    bool extensionsSupported = checkDeviceExtensionSupport(device);

    // We need a swapchain, so check if we have support
    bool swapChainAdequate = false;
    if (extensionsSupported) {
        SwapChainSupportDetails swapChainSupport =
            querySwapChainSupport(device, surface);
        // set to true if we have formats and present modes
        swapChainAdequate = !swapChainSupport.formats.empty() &&
                            !swapChainSupport.presentModes.empty();
    }

    // Make sure everything is true and we can mark the device as suitable
    return indices.isComplete() && extensionsSupported && swapChainAdequate;
}

// Return the best surface format for the swap chain
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

// Return the best present mode for the swap chain
VkPresentModeKHR chooseSwapPresentMode(
    const std::vector<VkPresentModeKHR> &availablePresentModes) {
    for (const auto &availablePresentMode : availablePresentModes) {
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return availablePresentMode;
        }
    }
    return VK_PRESENT_MODE_FIFO_KHR;
}

// Return the swap extent for our swap chain
VkExtent2D chooseSwapExtent(SDL_Window *window,
                            const VkSurfaceCapabilitiesKHR &capabilities) {
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

static std::vector<char> readFile(const std::string &filename) {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        consoleMessage("Failed to open shader file!", true);
    }

    size_t fileSize = (size_t)file.tellg();
    std::vector<char> buffer(fileSize);

    file.seekg(0);
    file.read(buffer.data(), fileSize);

    file.close();
    return buffer;
}

// Creates the shader module from shader code
VkShaderModule createShaderModule(VkDevice device,
                                  const std::vector<char> &code) {
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t *>(code.data());

    VkShaderModule shaderModule;
    if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) !=
        VK_SUCCESS) {
        consoleMessage("Failed to create shader module!", true);
    }

    return shaderModule;
}

void destroyDebugUtilsMessengerEXT(VkInstance instance,
                                   VkDebugUtilsMessengerEXT debugMessenger,
                                   const VkAllocationCallbacks *pAllocator) {
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
        instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
        func(instance, debugMessenger, pAllocator);
    }
}

void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex,
                         VkRenderPass renderPass,
                         std::vector<VkFramebuffer> swapChainFramebuffers,
                         VkExtent2D swapChainExtent,
                         VkPipeline graphicsPipeline) {
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
        consoleMessage("Failed to begin recording command buffer!", true);
    }

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = renderPass;
    renderPassInfo.framebuffer = swapChainFramebuffers[imageIndex];
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = swapChainExtent;

    VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearColor;

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo,
                         VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                      graphicsPipeline);

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)swapChainExtent.width;
    viewport.height = (float)swapChainExtent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = swapChainExtent;
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    vkCmdDraw(commandBuffer, 3, 1, 0, 0);
    vkCmdEndRenderPass(commandBuffer);

    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
        consoleMessage("Failed to record command buffer!", true);
    }
}

int main() {
    if (enableValidationLayers) {
        consoleMessage("\n[DEBUG MODE]", false);
    } else {
        consoleMessage("\n[RELEASE MODE]", false);
    }

    // STEP 0) INITIALIZING SDL2 ----------------------------------------------
    consoleMessage("\nBegin initializing SDL2...", false);

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        consoleMessage("Failed to initialize SDL2!", false);
        consoleMessage(SDL_GetError(), true);
    } else {
        consoleMessage("Successfully initialized SDL2 video", false);
    }

    SDL_Window *window =
        SDL_CreateWindow("Ape Escape Remake", SDL_WINDOWPOS_CENTERED,
                         SDL_WINDOWPOS_CENTERED, 800, 600, SDL_WINDOW_VULKAN);

    if (!window) {
        consoleMessage("Failed to create SDL2 window!", false);
        consoleMessage(SDL_GetError(), true);
    } else {
        consoleMessage("Successfully created SDL2 window", false);
    }

    consoleMessage("Successfully initialized SDL2\n", false);
    // STEP 0) INITIALIZING SDL2 ----------------------------------------------

    consoleMessage("Begin initializing Vulkan...\n", false);

    // STEP 1) INITIALIZING VULKAN INSTANCE -----------------------------------
    consoleMessage("Begin creating Vulkan instance...", false);
    VkInstance instance;

    // Check that the validation layers we want are supported
    if (enableValidationLayers && !checkValidationLayerSupport()) {
        consoleMessage("Validation layers requested but not available!", true);
    } else {
        consoleMessage("Validation layers are enabled and supported", false);
    }

    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Ape Escape Remake";
    appInfo.applicationVersion = VK_MAKE_API_VERSION(1, 0, 0, 0);
    appInfo.pEngineName = "Ape Escape Engine";
    appInfo.engineVersion = VK_MAKE_API_VERSION(1, 0, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_3;

    VkInstanceCreateInfo createInstanceInfo{};
    createInstanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInstanceInfo.pApplicationInfo = &appInfo;

    auto extensions = getRequiredExtensions(window);
    createInstanceInfo.enabledExtensionCount =
        static_cast<uint32_t>(extensions.size());
    createInstanceInfo.ppEnabledExtensionNames = extensions.data();

    // Validation error messages if we are in debug mode
    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
    if (enableValidationLayers) {
        createInstanceInfo.enabledLayerCount =
            static_cast<uint32_t>(validationLayers.size());
        createInstanceInfo.ppEnabledLayerNames = validationLayers.data();
        debugCreateInfo.sType =
            VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        debugCreateInfo.messageSeverity =
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        debugCreateInfo.messageType =
            VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        debugCreateInfo.pfnUserCallback = debugCallback;
        createInstanceInfo.pNext =
            (VkDebugUtilsMessengerCreateInfoEXT *)&debugCreateInfo;
    } else {
        createInstanceInfo.enabledLayerCount = 0;
        createInstanceInfo.pNext = nullptr;
    }

    if (vkCreateInstance(&createInstanceInfo, nullptr, &instance) !=
        VK_SUCCESS) {
        consoleMessage("Failed to create Vulkan instance!", true);
    } else {
        consoleMessage("Successfully created Vulkan instance", false);
    }
    // STEP 1) INITIALIZING VULKAN INSTANCE -----------------------------------

    // STEP 2) CREATING VULKAN DEBUG MESSENGER --------------------------------
    VkDebugUtilsMessengerEXT debugMessenger;
    if (enableValidationLayers) {
        consoleMessage("\nBegin creating Vulkan debug messenger...", false);
        VkDebugUtilsMessengerCreateInfoEXT createMessengerInfo{};
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

        if (createDebugUtilsMessengerEXT(instance, &createMessengerInfo,
                                         nullptr,
                                         &debugMessenger) != VK_SUCCESS) {
            consoleMessage("Failed to create Vulkan debug messenger!", true);
        } else {
            consoleMessage("Successfully created Vulkan debug messenger",
                           false);
        }
    }
    // STEP 2) CREATING VULKAN DEBUG MESSENGER --------------------------------

    // STEP 3) CREATING VULKAN SURFACE ----------------------------------------
    consoleMessage("\nBegin creating Vulkan window surface...", false);
    VkSurfaceKHR surface;
    SDL_bool surfaceResult =
        SDL_Vulkan_CreateSurface(window, instance, &surface);
    if (surfaceResult == SDL_FALSE) {
        consoleMessage("Failed to create Vulkan window surface!", false);
        consoleMessage(SDL_GetError(), true);
    } else {
        consoleMessage("Successfully created Vulkan window surface", false);
    }
    // STEP 3) CREATING VULKAN SURFACE ----------------------------------------

    // STEP 4) SELECTING PHYSICAL DEVICE --------------------------------------
    consoleMessage("\nBegin selecting physical device...", false);
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;

    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

    if (deviceCount == 0) {
        consoleMessage("Failed to find any GPUs with Vulkan support!", true);
    } else {
        consoleMessage("Found at least one GPU with Vulkan support", false);
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

    // Loop through all the devices until we find one that is suitable
    // Set the physical device to the suitable one
    for (const auto &device : devices) {
        if (isDeviceSuitable(device, surface)) {
            physicalDevice = device;
            break;
        }
    }

    if (physicalDevice == VK_NULL_HANDLE) {
        consoleMessage("Failed to find a suitable GPU!", true);
    } else {
        consoleMessage("Successfully selected physical device", false);
    }
    // STEP 4) SELECTING PHYSICAL DEVICE --------------------------------------

    // STEP 5) CREATING LOGICAL DEVICE ----------------------------------------
    consoleMessage("\nBegin creating logical device...", false);
    VkDevice device;
    VkQueue graphicsQueue;
    VkQueue presentQueue;

    QueueFamilyIndices indices = findQueueFamilies(physicalDevice, surface);

    // Store the graphics and present family
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = {indices.graphicsFamily.value(),
                                              indices.presentFamily.value()};

    // Loop through the queue families and make the create info structs
    float queuePriority = 1.0f;
    for (uint32_t queueFamily : uniqueQueueFamilies) {
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    VkPhysicalDeviceFeatures deviceFeatures{};

    VkDeviceCreateInfo createDeviceInfo{};
    createDeviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

    createDeviceInfo.queueCreateInfoCount =
        static_cast<uint32_t>(queueCreateInfos.size());
    createDeviceInfo.pQueueCreateInfos = queueCreateInfos.data();
    createDeviceInfo.pEnabledFeatures = &deviceFeatures;

    createDeviceInfo.enabledExtensionCount =
        static_cast<uint32_t>(deviceExtensions.size());

    createDeviceInfo.ppEnabledExtensionNames = deviceExtensions.data();

    if (enableValidationLayers) {
        createDeviceInfo.enabledLayerCount =
            static_cast<uint32_t>(validationLayers.size());
        createDeviceInfo.ppEnabledLayerNames = validationLayers.data();
    } else {
        createDeviceInfo.enabledLayerCount = 0;
    }

    if (vkCreateDevice(physicalDevice, &createDeviceInfo, nullptr, &device) !=
        VK_SUCCESS) {
        consoleMessage("Failed to create logical device!", true);
    } else {
        consoleMessage("Successfully created logical device", false);
    }

    vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &graphicsQueue);
    vkGetDeviceQueue(device, indices.presentFamily.value(), 0, &presentQueue);
    // STEP 5) CREATING LOGICAL DEVICE ----------------------------------------

    // STEP 6) CREATING SWAP CHAIN --------------------------------------------
    consoleMessage("\nBegin creating swap chain...", false);
    VkSwapchainKHR swapChain;
    std::vector<VkImage> swapChainImages;
    VkFormat swapChainImageFormat;
    VkExtent2D swapChainExtent;

    // Make sure we have swap chain support
    SwapChainSupportDetails swapChainSupport =
        querySwapChainSupport(physicalDevice, surface);

    // Grab the surface, present, and extent formats for the swap chain
    VkSurfaceFormatKHR surfaceFormat =
        chooseSwapSurfaceFormat(swapChainSupport.formats);
    VkPresentModeKHR presentMode =
        chooseSwapPresentMode(swapChainSupport.presentModes);
    VkExtent2D extent = chooseSwapExtent(window, swapChainSupport.capabilities);

    // The number of images we'll have in the swap chain
    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
    if (swapChainSupport.capabilities.maxImageCount > 0 &&
        imageCount > swapChainSupport.capabilities.maxImageCount) {
        imageCount = swapChainSupport.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createSwapInfo{};
    createSwapInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createSwapInfo.surface = surface;

    createSwapInfo.minImageCount = imageCount;
    createSwapInfo.imageFormat = surfaceFormat.format;
    createSwapInfo.imageColorSpace = surfaceFormat.colorSpace;
    createSwapInfo.imageExtent = extent;
    createSwapInfo.imageArrayLayers = 1;
    createSwapInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    QueueFamilyIndices indices2 = findQueueFamilies(physicalDevice, surface);
    uint32_t queueFamilyIndices[] = {indices2.graphicsFamily.value(),
                                     indices2.presentFamily.value()};

    if (indices.graphicsFamily != indices.presentFamily) {
        createSwapInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createSwapInfo.queueFamilyIndexCount = 2;
        createSwapInfo.pQueueFamilyIndices = queueFamilyIndices;
    } else {
        createSwapInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    createSwapInfo.preTransform =
        swapChainSupport.capabilities.currentTransform;
    createSwapInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createSwapInfo.presentMode = presentMode;
    createSwapInfo.clipped = VK_TRUE;

    createSwapInfo.oldSwapchain = VK_NULL_HANDLE;

    if (vkCreateSwapchainKHR(device, &createSwapInfo, nullptr, &swapChain) !=
        VK_SUCCESS) {
        consoleMessage("Failed to create swap chain!", true);
    } else {
        consoleMessage("Successfully created swap chain", false);
    }

    vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
    swapChainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(device, swapChain, &imageCount,
                            swapChainImages.data());

    swapChainImageFormat = surfaceFormat.format;
    swapChainExtent = extent;
    // STEP 6) CREATING SWAP CHAIN --------------------------------------------

    // STEP 7) CREATING IMAGE VIEWS -------------------------------------------
    consoleMessage("\nBegin creating image views...", false);
    std::vector<VkImageView> swapChainImageViews;
    swapChainImageViews.resize(swapChainImages.size());

    // Swap chain needs image views, create them
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
            consoleMessage("Failed to create image view!", true);
        } else {
            consoleMessage("Successfully created image view", false);
        }
    }
    consoleMessage("Successfully created all image views", false);
    // STEP 7) CREATING IMAGE VIEWS -------------------------------------------

    // STEP 8) CREATING RENDER PASS -------------------------------------------
    consoleMessage("\nBegin creating render pass...", false);

    // Just a bunch of structs to describe the render pass
    VkRenderPass renderPass;
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = swapChainImageFormat;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;

    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) !=
        VK_SUCCESS) {
        consoleMessage("Failed to create render pass!", true);
    } else {
        consoleMessage("Successfully created render pass", false);
    }
    // STEP 8) CREATING RENDER PASS -------------------------------------------

    // STEP 9) CREATING GRAPHICS PIPELINE -------------------------------------
    consoleMessage("\nBegin creating graphics pipeline...", false);
    VkPipelineLayout pipelineLayout;
    VkPipeline graphicsPipeline;

    // Our shader code
    auto vertShaderCode = readFile("vert.spv");
    auto fragShaderCode = readFile("frag.spv");

    VkShaderModule vertShaderModule =
        createShaderModule(device, vertShaderCode);
    VkShaderModule fragShaderModule =
        createShaderModule(device, fragShaderCode);

    // Create the structs which will contain the shader modules
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

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType =
        VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 0;
    vertexInputInfo.vertexAttributeDescriptionCount = 0;

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType =
        VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType =
        VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType =
        VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
        VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType =
        VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    colorBlending.blendConstants[0] = 0.0f;
    colorBlending.blendConstants[1] = 0.0f;
    colorBlending.blendConstants[2] = 0.0f;
    colorBlending.blendConstants[3] = 0.0f;

    std::vector<VkDynamicState> dynamicStates = {VK_DYNAMIC_STATE_VIEWPORT,
                                                 VK_DYNAMIC_STATE_SCISSOR};

    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount =
        static_cast<uint32_t>(dynamicStates.size());
    dynamicState.pDynamicStates = dynamicStates.data();

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 0;
    pipelineLayoutInfo.pushConstantRangeCount = 0;

    if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr,
                               &pipelineLayout) != VK_SUCCESS) {
        consoleMessage("Failed to create pipeline layout!", true);
    } else {
        consoleMessage("Successfully created pipeline layout", false);
    }

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = pipelineLayout;
    pipelineInfo.renderPass = renderPass;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo,
                                  nullptr, &graphicsPipeline) != VK_SUCCESS) {
        consoleMessage("Failed to create graphics pipeline!", true);
    } else {
        consoleMessage("Successfully created graphics pipeline", false);
    }

    vkDestroyShaderModule(device, fragShaderModule, nullptr);
    vkDestroyShaderModule(device, vertShaderModule, nullptr);
    // STEP 9) CREATING GRAPHICS PIPELINE -------------------------------------

    // STEP 10) CREATING FRAMEBUFFERS -----------------------------------------
    consoleMessage("\nBegin creating framebuffers...", false);
    std::vector<VkFramebuffer> swapChainFramebuffers;
    swapChainFramebuffers.resize(swapChainImageViews.size());

    // Need a framebuffer for each swap chain image view
    for (size_t i = 0; i < swapChainImageViews.size(); i++) {
        VkImageView attachments[] = {swapChainImageViews[i]};

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = renderPass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = attachments;
        framebufferInfo.width = swapChainExtent.width;
        framebufferInfo.height = swapChainExtent.height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(device, &framebufferInfo, nullptr,
                                &swapChainFramebuffers[i]) != VK_SUCCESS) {
            consoleMessage("Failed to create framebuffer!", true);
        } else {
            consoleMessage("Successfully created framebuffer", false);
        }
    }
    consoleMessage("Successfully created all framebuffers", false);
    // STEP 10) CREATING FRAMEBUFFERS -----------------------------------------

    // STEP 11) CREATING COMMAND POOL -----------------------------------------
    consoleMessage("\nBegin creating command pool...", false);
    VkCommandPool commandPool;
    QueueFamilyIndices queueFamilyIndices3 =
        findQueueFamilies(physicalDevice, surface);

    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = queueFamilyIndices3.graphicsFamily.value();

    if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) !=
        VK_SUCCESS) {
        consoleMessage("Failed to create command pool!", true);
    } else {
        consoleMessage("Successfully created command pool", false);
    }
    // STEP 11) CREATING COMMAND POOL -----------------------------------------

    // STEP 12) CREATING COMMAND BUFFERS --------------------------------------
    consoleMessage("\nBegin creating command buffers...", false);
    VkCommandBuffer commandBuffer;
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;

    if (vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer) !=
        VK_SUCCESS) {
        consoleMessage("Failed to create command buffers!", true);
    } else {
        consoleMessage("Successfully created command buffers", false);
    }
    // STEP 12) CREATING COMMAND BUFFERS --------------------------------------

    // STEP 13) CREATE SYNC OBJECTS -------------------------------------------
    consoleMessage("\nBegin creating sync objects...", false);
    VkSemaphore imageAvailableSemaphore;
    VkSemaphore renderFinishedSemaphore;
    VkFence inFlightFence;
    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    if (vkCreateSemaphore(device, &semaphoreInfo, nullptr,
                          &imageAvailableSemaphore) != VK_SUCCESS ||
        vkCreateSemaphore(device, &semaphoreInfo, nullptr,
                          &renderFinishedSemaphore) != VK_SUCCESS ||
        vkCreateFence(device, &fenceInfo, nullptr, &inFlightFence) !=
            VK_SUCCESS) {
        consoleMessage("Failed to create sync objects", true);
    } else {
        consoleMessage("Successfully created sync objects", false);
        consoleMessage("\nSuccessfully initialized Vulkan\n", false);
    }
    // STEP 13) CREATE SYNC OBJECTS -------------------------------------------

    // STEP 14) MAIN LOOP -----------------------------------------------------
    SDL_Event e;
    bool bQuit = false;
    consoleMessage("Begin program loop...", false);
    while (!bQuit) {
        while (SDL_PollEvent(&e)) {
            // If event type is quit or escape key is pressed, set bQuit to true
            if (e.type == SDL_QUIT ||
                (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE)) {
                bQuit = true;
            }

            vkWaitForFences(device, 1, &inFlightFence, VK_TRUE, UINT64_MAX);
            vkResetFences(device, 1, &inFlightFence);

            uint32_t imageIndex;
            vkAcquireNextImageKHR(device, swapChain, UINT64_MAX,
                                  imageAvailableSemaphore, VK_NULL_HANDLE,
                                  &imageIndex);

            vkResetCommandBuffer(commandBuffer, 0);
            recordCommandBuffer(commandBuffer, imageIndex, renderPass,
                                swapChainFramebuffers, swapChainExtent,
                                graphicsPipeline);

            VkSubmitInfo submitInfo{};
            submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

            VkSemaphore waitSemaphores[] = {imageAvailableSemaphore};
            VkPipelineStageFlags waitStages[] = {
                VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
            submitInfo.waitSemaphoreCount = 1;
            submitInfo.pWaitSemaphores = waitSemaphores;
            submitInfo.pWaitDstStageMask = waitStages;

            submitInfo.commandBufferCount = 1;
            submitInfo.pCommandBuffers = &commandBuffer;

            VkSemaphore signalSemaphores[] = {renderFinishedSemaphore};
            submitInfo.signalSemaphoreCount = 1;
            submitInfo.pSignalSemaphores = signalSemaphores;

            if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFence) !=
                VK_SUCCESS) {
                consoleMessage("Failed to submit draw command buffer!", true);
            }

            VkPresentInfoKHR presentInfo{};
            presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
            presentInfo.waitSemaphoreCount = 1;
            presentInfo.pWaitSemaphores = signalSemaphores;

            VkSwapchainKHR swapChains[] = {swapChain};
            presentInfo.swapchainCount = 1;
            presentInfo.pSwapchains = swapChains;
            presentInfo.pImageIndices = &imageIndex;

            vkQueuePresentKHR(presentQueue, &presentInfo);
        }
    }
    consoleMessage("Successfully exited program loop", false);
    consoleMessage("\nBegin shutdown process...", false);
    vkDeviceWaitIdle(device);
    // STEP 14) MAIN LOOP -----------------------------------------------------

    // STEP 15) CLEANUP -------------------------------------------------------
    vkDestroySemaphore(device, renderFinishedSemaphore, nullptr);
    vkDestroySemaphore(device, imageAvailableSemaphore, nullptr);
    vkDestroyFence(device, inFlightFence, nullptr);
    consoleMessage("Destroyed semaphores and fences", false);

    vkDestroyCommandPool(device, commandPool, nullptr);
    consoleMessage("Destroyed command pool", false);

    consoleMessage("Begin destroying framebuffers...", false);
    for (auto framebuffer : swapChainFramebuffers) {
        vkDestroyFramebuffer(device, framebuffer, nullptr);
        consoleMessage("Successfully destroyed framebuffer", false);
    }
    consoleMessage("Destroyed all framebuffers", false);

    vkDestroyPipeline(device, graphicsPipeline, nullptr);
    vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
    consoleMessage("Destroyed graphics pipeline and layout", false);

    vkDestroyRenderPass(device, renderPass, nullptr);
    consoleMessage("Destroyed render pass", false);

    consoleMessage("Begin destroying image views...", false);
    for (auto imageView : swapChainImageViews) {
        vkDestroyImageView(device, imageView, nullptr);
        consoleMessage("Successfully destroyed image view", false);
    }
    consoleMessage("Destroyed all image views", false);

    vkDestroySwapchainKHR(device, swapChain, nullptr);
    consoleMessage("Destroyed swap chain", false);

    vkDestroyDevice(device, nullptr);
    consoleMessage("Destroyed logical device", false);

    if (enableValidationLayers) {
        destroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
        consoleMessage("Destroyed debug messenger", false);
    }

    vkDestroySurfaceKHR(instance, surface, nullptr);
    consoleMessage("Destroyed surface", false);
    vkDestroyInstance(instance, nullptr);
    consoleMessage("Destroyed instance", false);

    SDL_DestroyWindowSurface(window);
    SDL_DestroyWindow(window);
    consoleMessage("Destroyed SDL2 window surface and window", false);
    SDL_Quit();
    consoleMessage("Destroyed SDL2", false);
    consoleMessage("Successfully shutdown program", false);
    return 0;
    // STEP 15) CLEANUP -------------------------------------------------------
}