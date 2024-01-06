#ifndef VULKAN_CONTEXT_H
#define VULKAN_CONTEXT_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>
#include <vulkan/vulkan.h>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <optional>
#include <set>
#include <vector>

#include "core/debugger/debugger.h"

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

const int MAX_FRAMES_IN_FLIGHT = 2;

struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;

    bool isComplete() {
        return graphicsFamily.has_value() && presentFamily.has_value();
    }
};

struct SwapchainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

class VulkanContext {
   public:
    // Grab the SDL2 window from the display server
    void setWindow(SDL_Window* window);

    // Initialize Vulkan by calling all the helper functions
    void initVulkan();
    void cleanup();
    void drawFrame();

   private:
    // Get the queue families for the physical device
    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);

    Debugger debugger;
    SDL_Window* window = NULL;

    VkInstance instance;
    VkDebugUtilsMessengerEXT debugMessenger;
    VkSurfaceKHR surface;

    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device;

    VkQueue graphicsQueue;
    VkQueue presentQueue;

    VkSwapchainKHR swapchain;
    std::vector<VkImage> swapchainImages;
    VkFormat swapchainImageFormat;
    VkExtent2D swapchainExtent;
    std::vector<VkImageView> swapchainImageViews;
    std::vector<VkFramebuffer> swapchainFramebuffers;

    VkRenderPass renderPass;
    VkPipelineLayout pipelineLayout;
    VkPipeline graphicsPipeline;

    VkCommandPool commandPool;
    std::vector<VkCommandBuffer> commandBuffers;

    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> inFlightFences;
    uint32_t currentFrame = 0;

    bool framebufferResized = false;

    // The steps in order we take to initialize Vulkan
    void createInstance();
    // If debug mode, create the debug messenger
    void setupDebugMessenger();
    void createSurface();
    void pickPhysicalDevice();
    void createLogicalDevice();
    void createSwapchain();
    void createImageViews();
    void createRenderPass();
    void createGraphicsPipeline();
    void createFramebuffers();
    void createCommandPool();
    void createCommandBuffers();
    void createSyncObjects();

    // Check to make sure we have the required validation layers
    bool checkValidationLayerSupport();

    // Get the required extensions for the Vulkan instance
    std::vector<const char*> getRequiredExtensions();

    // Create a debug messenger struct to handle Vulkan validation layers
    void populateDebugMessengerCreateInfo(
        VkDebugUtilsMessengerCreateInfoEXT& createInfo);

    static VKAPI_ATTR VkBool32 VKAPI_CALL
    debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                  VkDebugUtilsMessageTypeFlagsEXT messaageType,
                  const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                  void* pUserData) {
        std::cerr << "Validation layer: " << pCallbackData->pMessage
                  << std::endl;
        return VK_FALSE;
    }

    // Create the debug messenger
    VkResult createDebugUtilsMessengerEXT(
        VkInstance instance,
        const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
        const VkAllocationCallbacks* pAllocator,
        VkDebugUtilsMessengerEXT* pDebugMessenger);

    // Destroy the debug messenger
    void destroyDebugUtilsMessengerEXT(VkInstance instance,
                                       VkDebugUtilsMessengerEXT debugMessenger,
                                       const VkAllocationCallbacks* pAllocator);

    // Check to make sure the physical device has all we need for Vulkan
    bool isDeviceSuitable(VkPhysicalDevice device);
    // Check to make sure the physical device has the required extensions
    bool checkDeviceExtensionSupport(VkPhysicalDevice device);
    // Get the swapchain support details for the physical device
    SwapchainSupportDetails querySwapchainSupport(VkPhysicalDevice device);

    // Get the desired surface format
    VkSurfaceFormatKHR chooseSwapSurfaceFormat(
        const std::vector<VkSurfaceFormatKHR>& availableFormats);
    // Get the desired present mode
    VkPresentModeKHR chooseSwapPresentMode(
        const std::vector<VkPresentModeKHR>& availablePresentModes);
    // Get the desired swap extent
    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);

    // Read in a file and return the buffer
    std::vector<char> readFile(const std::string& filename);

    // Destroy the swap chain
    void cleanupSwapchain();

    // Create a shader module from a buffer
    VkShaderModule createShaderModule(const std::vector<char>& code);

    // If the window is resized, we need to recreate the swap chain
    void recreateSwapchain();

    void recordCommandBuffer(VkCommandBuffer commandBuffer,
                             uint32_t imageIndex);

};

#endif