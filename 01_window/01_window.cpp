#include <algorithm>
#include <cstdio>
#include <stdexcept>
#include <vector>

#define GLFW_INCLUDE_VULKAN
#define GLFW_INCLUDE_NONE
#include <vulkan/vulkan.h>

#include <GLFW/glfw3.h>

#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>

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
    const VkPhysicalDevice      phyDevice,
    const VkDevice              device,
    const VkSurfaceKHR          surface,
    const VkSurfaceFormatKHR    surfaceFormat,
    const uint32_t              width,
    const uint32_t              height,
    VkSwapchainKHR*             outSwapchain) {

    VkSurfaceCapabilitiesKHR capabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(phyDevice, surface, &capabilities);

    uint32_t imageCount = capabilities.minImageCount;
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
        .preTransform           = capabilities.currentTransform,
        .compositeAlpha         = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode            = presentMode,
        .clipped                = VK_TRUE,
        .oldSwapchain           = VK_NULL_HANDLE,
    };

    return vkCreateSwapchainKHR(device, &createInfo, nullptr, outSwapchain);
}

std::vector<VkImage> GetSwapchainImages(const VkDevice device, const VkSwapchainKHR swapchain) {
    uint32_t imageCount = 0;
    vkGetSwapchainImagesKHR(device, swapchain, &imageCount, nullptr);

    std::vector<VkImage> images(imageCount);
    vkGetSwapchainImagesKHR(device, swapchain, &imageCount, images.data());

    return images;
}

void DestroyImageViews(const VkDevice device, std::vector<VkImageView>& views) {
    for (size_t idx = 0; idx < views.size(); idx++) {
        vkDestroyImageView(device, views[idx], nullptr);
        views[idx] = VK_NULL_HANDLE;
    }
}

std::vector<VkImageView> Create2DImageViews(
    const VkDevice              device,
    const VkFormat              format,
    const std::vector<VkImage>& images) {

    VkImageViewCreateInfo createInfo = {
        .sType          = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .pNext          = nullptr,
        .flags          = 0,
        .image          = VK_NULL_HANDLE,   // will be updated below
        .viewType       = VK_IMAGE_VIEW_TYPE_2D,
        .format         = format,
        .components     = {
            .r = VK_COMPONENT_SWIZZLE_IDENTITY,
            .g = VK_COMPONENT_SWIZZLE_IDENTITY,
            .b = VK_COMPONENT_SWIZZLE_IDENTITY,
            .a = VK_COMPONENT_SWIZZLE_IDENTITY },
        .subresourceRange = {
            .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel   = 0,
            .levelCount     = 1,
            .baseArrayLayer = 0,
            .layerCount     = 1,
        }
    };

    std::vector<VkImageView> views(images.size(), VK_NULL_HANDLE);

    for (size_t idx = 0; idx < images.size(); idx++) {
        createInfo.image = images[idx];


        VkResult result = vkCreateImageView(device, &createInfo, nullptr, &views[idx]);
        if (result != VK_SUCCESS) {
            DestroyImageViews(device, views);
            return {};
        }
    }

    return views;
}

VkResult CreateCommandPool(const VkDevice device, const uint32_t queueFamilyIdx, VkCommandPool* outCmdPool) {
    VkCommandPoolCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .pNext = nullptr,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = queueFamilyIdx,
    };

    return vkCreateCommandPool(device, &createInfo, nullptr, outCmdPool);
}

VkResult CreateSimpleRenderPass(
    const VkDevice      device,
    const VkFormat      colorFormat,
    VkRenderPass*       outRenderPass) {

    const VkAttachmentDescription colorAttachments[] = {
        {
            .flags          = 0,
            .format         = colorFormat,
            .samples        = VK_SAMPLE_COUNT_1_BIT,
            .loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp        = VK_ATTACHMENT_STORE_OP_STORE,
            .stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED,
            .finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        },
    };

    VkAttachmentReference colorAttachmentRef = {
        .attachment = 0,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    };

    const VkSubpassDescription subpass = {
        .flags                       = 0,
        .pipelineBindPoint          = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .inputAttachmentCount       = 0,
        .pInputAttachments          = NULL,
        .colorAttachmentCount       = 1,
        .pColorAttachments          = &colorAttachmentRef,
        .pResolveAttachments        = NULL,
        .pDepthStencilAttachment    = NULL,
        .preserveAttachmentCount    = 0,
        .pPreserveAttachments       = NULL,
    };

    VkRenderPassCreateInfo createInfo = {
        .sType              = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .pNext              = nullptr,
        .flags              = 0,
        .attachmentCount    = std::size(colorAttachments),
        .pAttachments       = colorAttachments,
        .subpassCount       = 1,
        .pSubpasses         = &subpass,
        .dependencyCount    = 0,
        .pDependencies      = NULL,
    };

    return vkCreateRenderPass(device, &createInfo, nullptr, outRenderPass);
}


std::vector<VkCommandBuffer> AllocateCommandBuffers(
    const VkDevice          device,
    const VkCommandPool     cmdPool,
    const uint32_t          count) {
    std::vector<VkCommandBuffer> cmdBuffers(count, VK_NULL_HANDLE);

    VkCommandBufferAllocateInfo allocInfo = {
        .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .pNext              = nullptr,
        .commandPool        = cmdPool,
        .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = (uint32_t)cmdBuffers.size(),
    };

    // TODO: check result
    vkAllocateCommandBuffers(device, &allocInfo, cmdBuffers.data());

    return cmdBuffers;
}

void DestroyFramebuffers(const VkDevice device, std::vector<VkFramebuffer>& framebuffers) {
    for (size_t idx = 0; idx < framebuffers.size(); idx++) {
        vkDestroyFramebuffer(device, framebuffers[idx], nullptr);
        framebuffers[idx] = VK_NULL_HANDLE;
    }
}

std::vector<VkFramebuffer> CreateSimpleFramebuffers(
    const VkDevice                  device,
    const VkRenderPass              renderPass,
    const uint32_t                  width,
    const uint32_t                  height,
    const std::vector<VkImageView>& renderViews) {

    std::vector<VkFramebuffer> framebuffers(renderViews.size(), VK_NULL_HANDLE);

    VkFramebufferCreateInfo createInfo = {
        .sType              = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .pNext              = NULL,
        .flags              = 0,
        .renderPass         = renderPass,
        .attachmentCount    = 1,
        .pAttachments       = nullptr, // specified below
        .width              = width,
        .height             = height,
        .layers             = 1,
    };

    for (size_t idx = 0; idx < renderViews.size(); idx++) {
        createInfo.pAttachments = &renderViews[idx];

        VkResult result = vkCreateFramebuffer(device, &createInfo, nullptr, &framebuffers[idx]);
        if (result != VK_SUCCESS) {
            DestroyFramebuffers(device, framebuffers);
            return {};
        }
    }

    return framebuffers;
}


VkFence CreateFence(const VkDevice device) {
   VkFenceCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .pNext = 0,
        .flags = VK_FENCE_CREATE_SIGNALED_BIT,
    };

    VkFence fence = VK_NULL_HANDLE;
    vkCreateFence(device, &createInfo, nullptr, &fence);

    return fence;
}

VkSemaphore CreateSemaphore(const VkDevice device) {
    VkSemaphoreCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
    };

    VkSemaphore semaphore = VK_NULL_HANDLE;
    vkCreateSemaphore(device, &createInfo, nullptr, &semaphore);

    return semaphore;
}

VkResult CreateSimpleDescriptorPool(const VkDevice device, VkDescriptorPool *outDescPool) {

    VkDescriptorPoolSize poolSizes[] = {
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1 },
    };

    VkDescriptorPoolCreateInfo createInfo = {
        .sType          = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .pNext          = nullptr,
        .flags          = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
        .maxSets        = 1,
        .poolSizeCount  = std::size(poolSizes),
        .pPoolSizes     = poolSizes,
    };
    return vkCreateDescriptorPool(device, &createInfo, nullptr, outDescPool);
}

void KeyCallback(GLFWwindow* window, int key, int /*scancode*/, int /*action*/, int /*mods*/) {
    switch (key) {
        case GLFW_KEY_ESCAPE: {
            glfwSetWindowShouldClose(window, GLFW_TRUE);
            break;
        }
    }
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

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    (void)io;

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
    uint32_t windowWidth  = 1024;
    uint32_t windowHeight = 800;
    GLFWwindow *window    = glfwCreateWindow(windowWidth, windowHeight, "01_window GLFW", NULL, NULL);

    glfwSetWindowUserPointer(window, nullptr);
    glfwSetKeyCallback(window, KeyCallback);

    ImGui_ImplGlfw_InitForVulkan(window, true);

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

    VkSurfaceFormatKHR surfaceInfo = {};
    FindGoodSurfaceFormat(phyDevice, surface, preferredFormats, &surfaceInfo);

    VkSwapchainKHR swapchain = VK_NULL_HANDLE;
    CreateSwapchain(phyDevice, device, surface, surfaceInfo, windowWidth, windowHeight, &swapchain); // TODO: check result

    std::vector<VkImage> swapchainImages    = GetSwapchainImages(device, swapchain);
    std::vector<VkImageView> swapchainViews = Create2DImageViews(device, surfaceInfo.format, swapchainImages);

    VkCommandPool cmdPool = VK_NULL_HANDLE;
    CreateCommandPool(device, queueFamilyIdx, &cmdPool); // TODO: check result

    std::vector<VkCommandBuffer> cmdBuffers = AllocateCommandBuffers(device, cmdPool, swapchainImages.size());

    VkRenderPass renderPass = VK_NULL_HANDLE;
    CreateSimpleRenderPass(device, surfaceInfo.format, &renderPass); // TODO: check result

    VkDescriptorPool descPool = VK_NULL_HANDLE;
    CreateSimpleDescriptorPool(device, &descPool); // TODO: check result

    {
        ImGui_ImplVulkan_InitInfo imguiInfo = {
            .Instance       = instance,
            .PhysicalDevice = phyDevice,
            .Device         = device,
            .QueueFamily    = queueFamilyIdx,
            .Queue          = queue,
            .DescriptorPool = descPool,
            .RenderPass     = renderPass,
            .MinImageCount  = 2,
            .ImageCount     = 2,
            .MSAASamples    = VK_SAMPLE_COUNT_1_BIT,
            .PipelineCache  = VK_NULL_HANDLE,
            .Subpass        = 0,

            .UseDynamicRendering            = false,
            .PipelineRenderingCreateInfo    = {},
            .Allocator                      = nullptr,
            .CheckVkResultFn                = nullptr,
            .MinAllocationSize              = 1024*1024,
        };
        ImGui_ImplVulkan_Init(&imguiInfo);

        ImGui_ImplVulkan_CreateFontsTexture();
    }

    std::vector<VkFramebuffer> framebuffers = CreateSimpleFramebuffers(device, renderPass, windowWidth, windowHeight, swapchainViews);

    VkFence imageFence              = CreateFence(device);
    VkSemaphore presentSemaphore    = CreateSemaphore(device);

    glfwShowWindow(window);

    int32_t color = 0;
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        {
            ImGui_ImplVulkan_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

            ImGui::ShowDemoWindow();

            ImGui::Begin("Info");

            static bool colorAutoInc = true;
            ImGui::Checkbox("Use auto increment", &colorAutoInc);

            if (colorAutoInc) {
                color = (color + 1) % 255;
            }

            ImGui::SliderInt("Red value", &color, 0, 255);

            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);

            ImGui::End();

            ImGui::Render();
        }

        vkResetFences(device, 1, &imageFence);

        uint32_t swapchainIdx = -1;
        vkAcquireNextImageKHR(device, swapchain, -1, VK_NULL_HANDLE, imageFence, &swapchainIdx);
        vkWaitForFences(device, 1, &imageFence, VK_TRUE, UINT64_MAX);


        VkCommandBuffer cmdBuffer = cmdBuffers[swapchainIdx];

        {
            VkCommandBufferBeginInfo beginInfo = {
                .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
                .pNext              = nullptr,
                .flags              = 0,
                .pInheritanceInfo   = nullptr,
            };

            vkBeginCommandBuffer(cmdBuffer, &beginInfo);

            VkClearValue clearColor = { { { color / 255.0f, 0.0f, 0.0f, 1.0f } } };
            VkRenderPassBeginInfo renderPassInfo = {
                .sType          = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
                .pNext          = nullptr,
                .renderPass     = renderPass,
                .framebuffer    = framebuffers[swapchainIdx],
                .renderArea     = {
                    .offset = { 0, 0 },
                    .extent = { (uint32_t)windowWidth, (uint32_t)windowHeight },
                },
                .clearValueCount = 1,
                .pClearValues = &clearColor,
            };

            vkCmdBeginRenderPass(cmdBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

            ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmdBuffer);
            vkCmdEndRenderPass(cmdBuffer);

            vkEndCommandBuffer(cmdBuffer);
        }

        VkSubmitInfo submitInfo = {
            .sType                  = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .pNext                  = nullptr,
            .waitSemaphoreCount     = 0,
            .pWaitSemaphores        = nullptr,
            .pWaitDstStageMask      = nullptr,
            .commandBufferCount     = 1,
            .pCommandBuffers        = &cmdBuffer,
            .signalSemaphoreCount   = 1,
            .pSignalSemaphores      = &presentSemaphore,
        };

        vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);

        VkPresentInfoKHR presentInfo = {
            .sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
            .pNext              = 0,
            .waitSemaphoreCount = 1,
            .pWaitSemaphores    = &presentSemaphore,
            .swapchainCount     = 1,
            .pSwapchains        = &swapchain,
            .pImageIndices      = &swapchainIdx,
            .pResults           = nullptr,
        };

        vkQueuePresentKHR(queue, &presentInfo);

        vkDeviceWaitIdle(device);
    }

    {
        ImGui_ImplVulkan_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
    }

    vkDestroyFence(device, imageFence, nullptr);
    vkDestroySemaphore(device, presentSemaphore, nullptr);

    DestroyFramebuffers(device, framebuffers);
    vkDestroyRenderPass(device, renderPass, nullptr);

    vkDestroyDescriptorPool(device, descPool, nullptr);
    vkDestroyCommandPool(device, cmdPool, nullptr);

    DestroyImageViews(device, swapchainViews);
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
