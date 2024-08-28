#include <algorithm>
#include <cstdio>
#include <stdexcept>
#include <vector>

#define GLFW_INCLUDE_VULKAN
#define GLFW_INCLUDE_NONE
#include <vulkan/vulkan.h>

#include <GLFW/glfw3.h>

#define VK_LOAD_INSTANCE_PFN(instance, name) reinterpret_cast<PFN_##name>(vkGetInstanceProcAddr(instance, #name))

static VkBool32 DebugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT           /*messageSeverity*/,
    VkDebugUtilsMessageTypeFlagsEXT                  /*messageTypes*/,
    const VkDebugUtilsMessengerCallbackDataEXT*      pCallbackData,
    void*                                            /*pUserData*/) {

    fprintf(stderr, "[DebugCallback] %s\n", pCallbackData->pMessage);

    return VK_FALSE;
}

VkDebugUtilsMessengerCreateInfoEXT BuildDebugCallbackInfo(
    PFN_vkDebugUtilsMessengerCallbackEXT callback,
    void* userData) {
    /*
    typedef struct VkDebugUtilsMessengerCreateInfoEXT {
        VkStructureType                         sType;
        const void*                             pNext;
        VkDebugUtilsMessengerCreateFlagsEXT     flags;
        VkDebugUtilsMessageSeverityFlagsEXT     messageSeverity;
        VkDebugUtilsMessageTypeFlagsEXT         messageType;
        PFN_vkDebugUtilsMessengerCallbackEXT    pfnUserCallback;
        void*                                   pUserData;
    } VkDebugUtilsMessengerCreateInfoEXT;
    */
    return {
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
        .pNext = nullptr,
        .flags = 0,
        .messageSeverity = 0
            //| VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT
            //| VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT
            | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
            | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
        .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
            | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT,
        .pfnUserCallback = callback,
        .pUserData = userData,
    };
}

VkResult InitializeDebugCallback(
    VkInstance                              instance,
    PFN_vkDebugUtilsMessengerCallbackEXT    callback,
    void*                                   userData,
    VkDebugUtilsMessengerEXT*               debugMessenger) {

    auto vkCreateDebugUtilsMessengerEXT = VK_LOAD_INSTANCE_PFN(instance, vkCreateDebugUtilsMessengerEXT);

    VkDebugUtilsMessengerCreateInfoEXT debugInfo = BuildDebugCallbackInfo(callback, userData);
    return vkCreateDebugUtilsMessengerEXT(instance, &debugInfo, nullptr, debugMessenger);
}

VkResult CreateVkInstance(const std::vector<const char *> &extraExtensions, bool useValidation,
                          VkInstance *outInstance) {
    const std::vector<const char *> debugExtensions = {VK_EXT_DEBUG_UTILS_EXTENSION_NAME};
    const std::vector<const char *> debugLayers     = {"VK_LAYER_KHRONOS_validation"};

    std::vector<const char *> extensions = extraExtensions;
    std::ranges::copy(debugExtensions, std::back_inserter(extensions));

    VkDebugUtilsMessengerCreateInfoEXT debugInfo = BuildDebugCallbackInfo(DebugCallback, nullptr);

    /*
    typedef struct VkApplicationInfo {
        VkStructureType    sType;
        const void*        pNext;
        const char*        pApplicationName;
        uint32_t           applicationVersion;
        const char*        pEngineName;
        uint32_t           engineVersion;
        uint32_t           apiVersion;
    } VkApplicationInfo;
    */
    VkApplicationInfo appInfo = {
        .sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pNext              = nullptr,
        .pApplicationName   = "01_window",
        .applicationVersion = 1,
        .pEngineName        = "vkcourse",
        .engineVersion      = 1,
        .apiVersion         = VK_API_VERSION_1_1,
    };

    /*
    typedef struct VkInstanceCreateInfo {
        VkStructureType             sType;
        const void*                 pNext;
        VkInstanceCreateFlags       flags;
        const VkApplicationInfo*    pApplicationInfo;
        uint32_t                    enabledLayerCount;
        const char* const*          ppEnabledLayerNames;
        uint32_t                    enabledExtensionCount;
        const char* const*          ppEnabledExtensionNames;
    } VkInstanceCreateInfo;
    */
    VkInstanceCreateInfo info = {
        .sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pNext                   = &debugInfo,
        .flags                   = 0,
        .pApplicationInfo        = &appInfo,
        .enabledLayerCount       = useValidation ? 1u : 0u,
        .ppEnabledLayerNames     = debugLayers.data(),
        .enabledExtensionCount   = (uint32_t)extensions.size(),
        .ppEnabledExtensionNames = extensions.data(),
    };

    VkResult result = vkCreateInstance(&info, nullptr, outInstance);

    return result;
}

bool FindQueueFamily(const VkPhysicalDevice device, const VkSurfaceKHR surface, uint32_t *outQueueFamilyIdx) {
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    for (uint32_t idx = 0; idx < queueFamilyCount; idx++) {
        if (queueFamilies[idx].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            // Check if the selected graphics queue family supports presentation.
            // At the moment the example expects that the graphics and presentation queue is the same.
            // This is not always the case.
            // TODO: add support for different graphics and presentation family indices.
            VkBool32 presentSupport = VK_FALSE;
            vkGetPhysicalDeviceSurfaceSupportKHR(device, idx, surface, &presentSupport);

            if (presentSupport) {
                *outQueueFamilyIdx = idx;
                return true;
            }
        }
    }

    return false;
}

VkResult FindPhyDevice(const VkInstance instance, const VkSurfaceKHR surface, VkPhysicalDevice *outPhyDevice, uint32_t *outQueueFamilyIdx) {
    // Query the number of physical devices
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

    if (deviceCount == 0) {
        return VK_ERROR_INITIALIZATION_FAILED;
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

    // Iterate over the devices and find first device and bail
    for (const VkPhysicalDevice& device : devices) {
        if (FindQueueFamily(device, surface, outQueueFamilyIdx)) {
            *outPhyDevice = device;
            return VK_SUCCESS;
        }
    }

    return VK_ERROR_INITIALIZATION_FAILED;
}

void PrintPhyDeviceInfo(const VkInstance /*instance*/, const VkPhysicalDevice phyDevice) {
    VkPhysicalDeviceProperties properties = {};
    vkGetPhysicalDeviceProperties(phyDevice, &properties);

    uint32_t apiMajor = VK_API_VERSION_MAJOR(properties.apiVersion);
    uint32_t apiMinor = VK_API_VERSION_MINOR(properties.apiVersion);

    printf("Device Info: %s Vulkan API Version: %u.%u\n", properties.deviceName, apiMajor, apiMinor);
}

VkResult CreateDevice(
    const VkInstance                    /*instance*/,
    const VkPhysicalDevice              phyDevice,
    const uint32_t                      queueFamilyIdx,
    const std::vector<const char *>&    extraExtensions,
    VkDevice*                           outDevice) {

    const std::vector<const char *> swapchainExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

    std::vector<const char *> extensions = extraExtensions;
    std::ranges::copy(swapchainExtensions, std::back_inserter(extensions));

    const float queuePriority[1] = { 1.0f };

    VkDeviceQueueCreateInfo queueInfo = {
        .sType              = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .pNext              = nullptr,
        .flags              = 0,
        .queueFamilyIndex   = queueFamilyIdx,
        .queueCount         = 1,
        .pQueuePriorities   = queuePriority,
    };

    VkDeviceCreateInfo createInfo = {
        .sType                      = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext                      = nullptr,
        .flags                      = 0,
        .queueCreateInfoCount       = 1,
        .pQueueCreateInfos          = &queueInfo,
        .enabledLayerCount          = 0,        // deprecated
        .ppEnabledLayerNames        = nullptr,  // deprecated
        .enabledExtensionCount      = (uint32_t)extensions.size(),
        .ppEnabledExtensionNames    = extensions.data(),
        .pEnabledFeatures           = nullptr,
    };

    return vkCreateDevice(phyDevice, &createInfo, nullptr, outDevice);
}

bool FindGoodSurfaceFormat(
    const VkPhysicalDevice          phyDevice,
    const VkSurfaceKHR              surface,
    const std::vector<VkFormat>&    preferredFormats,
    VkSurfaceFormatKHR*             outSelectedSurfaceFormat) {

    uint32_t formatCount = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(phyDevice, surface, &formatCount, NULL);

    if (formatCount < 1) {
        return false;
    }

    std::vector<VkSurfaceFormatKHR> surfaceFormats(formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(phyDevice, surface, &formatCount, surfaceFormats.data());

    *outSelectedSurfaceFormat = surfaceFormats[0];

    for (const VkSurfaceFormatKHR& entry : surfaceFormats) {
        if ((std::find(preferredFormats.begin(), preferredFormats.end(), entry.format) != preferredFormats.end())
            && (entry.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)) {
            *outSelectedSurfaceFormat = entry;
            break;
        }
    }

    return true;
}

VkResult CreateSwapchain(
    const VkDevice              device,
    const VkSurfaceKHR          surface,
    const VkSurfaceFormatKHR    surfaceFormat,
    const uint32_t              width,
    const uint32_t              height,
    VkSwapchainKHR*             outSwapchain) {

    uint32_t imageCount = 4;
    const VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;

    VkSwapchainCreateInfoKHR createInfo = {
        .sType                  = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .pNext                  = 0,
        .flags                  = 0,
        .surface                = surface,
        .minImageCount          = imageCount,
        .imageFormat            = surfaceFormat.format,
        .imageColorSpace        = surfaceFormat.colorSpace,
        .imageExtent            = { width, height },
        .imageArrayLayers       = 1,
        .imageUsage             = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
        .imageSharingMode       = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount  = 0,
        .pQueueFamilyIndices    = nullptr,
        .preTransform           = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
        .compositeAlpha         = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode            = presentMode,
        .clipped                = VK_TRUE,
        .oldSwapchain           = VK_NULL_HANDLE,
    };

    return vkCreateSwapchainKHR(device, &createInfo, nullptr, outSwapchain);
}


int main(int /*argc*/, char **/*argv*/) {

    if (glfwVulkanSupported()) {
        printf("Failed to look up minimal Vulkan loader/ICD\n!");
        return -1;
    }

    if (!glfwInit()) {
        printf("Failed to init GLFW!\n");
        return -1;
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    uint32_t count              = 0;
    const char **glfwExtensions = glfwGetRequiredInstanceExtensions(&count);

    printf("Minimal set of requred extension by GLFW:\n");
    for (uint32_t idx = 0; idx < count; idx++) {
        printf("-> %s\n", glfwExtensions[idx]);
    }

    std::vector<const char *> extensions(glfwExtensions, glfwExtensions + count);

    VkInstance instance     = VK_NULL_HANDLE;
    VkResult instanceCreate = CreateVkInstance(extensions, true, &instance);
    if (instanceCreate != VK_SUCCESS) {
        printf("Failed to create Vulkan context! (error code: %d)\n", instanceCreate);
        return -1;
    }

    VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;
    InitializeDebugCallback(instance, DebugCallback, nullptr, &debugMessenger);

    // Create the window to render onto
    uint32_t windowWidth  = 512;
    uint32_t windowHeight = 512;
    GLFWwindow *window    = glfwCreateWindow(windowWidth, windowHeight, "01_window GLFW", NULL, NULL);

    // We have the window, the instance, create a surface from the window to draw onto.
    // Create a Vulkan Surface using GLFW.
    // By using GLFW the current windowing system's surface is created (xcb, win32, etc..)
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    if (glfwCreateWindowSurface(instance, window, NULL, &surface) != VK_SUCCESS) {
        // TODO: not the best, but will fail the application surely
        throw std::runtime_error("Failed to create window surface!");
    }

    VkPhysicalDevice phyDevice  = VK_NULL_HANDLE;
    uint32_t queueFamilyIdx     = -1;
    if (FindPhyDevice(instance, surface, &phyDevice, &queueFamilyIdx) != VK_SUCCESS) {
        throw std::runtime_error("Failed to find a good device/queue family");
    }

    PrintPhyDeviceInfo(instance, phyDevice);

    VkDevice device = VK_NULL_HANDLE;
    if (CreateDevice(instance, phyDevice, queueFamilyIdx, {}, &device) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create Vulkan Device\n");
    }

    VkQueue queue = VK_NULL_HANDLE;
    vkGetDeviceQueue(device, queueFamilyIdx, 0, &queue);

    const std::vector<VkFormat> preferredFormats
        = {VK_FORMAT_B8G8R8A8_SRGB, VK_FORMAT_R8G8B8A8_SRGB, VK_FORMAT_B8G8R8A8_UNORM, VK_FORMAT_R8G8B8A8_UNORM};

    VkSurfaceFormatKHR selectedSurfaceFormat = {};
    FindGoodSurfaceFormat(phyDevice, surface, preferredFormats, &selectedSurfaceFormat);

    VkSwapchainKHR swapchain = VK_NULL_HANDLE;
    CreateSwapchain(device, surface, selectedSurfaceFormat, windowWidth, windowHeight, &swapchain);


    vkDestroySwapchainKHR(device, swapchain, nullptr);
    vkDestroyDevice(device, nullptr);
    vkDestroySurfaceKHR(instance, surface, nullptr);

    // TODO: this is not nice (loading the func ptr and using it directly
    VK_LOAD_INSTANCE_PFN(instance, vkDestroyDebugUtilsMessengerEXT)(instance, debugMessenger, nullptr);
    vkDestroyInstance(instance, nullptr);

    glfwDestroyWindow(window);

    glfwTerminate();
    return 0;
}
