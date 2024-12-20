#include <algorithm>
#include <cstdio>
#include <stdexcept>
#include <vector>

#include <cmath>

#define GLFW_INCLUDE_VULKAN
#define GLFW_INCLUDE_NONE
#include <vulkan/vulkan.h>

#include <GLFW/glfw3.h>

#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace {
#include "triangle_in.vert_include.h"
#include "triangle_in.frag_include.h"
}

#include "buffer.h"
#include "descriptors.h"
#include "grid.h"
#include "shader_tooling.h"

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
    extensions.insert(extensions.end(), debugExtensions.begin(), debugExtensions.end());

    VkDebugUtilsMessengerCreateInfoEXT debugInfo =
        BuildDebugCallbackInfo(static_cast<PFN_vkDebugUtilsMessengerCallbackEXT>(DebugCallback), nullptr);

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
    extensions.insert(extensions.end(), swapchainExtensions.begin(), swapchainExtensions.end());

    const float queuePriority[1] = { 1.0f };

    VkDeviceQueueCreateInfo queueInfo = {
        .sType              = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .pNext              = nullptr,
        .flags              = 0,
        .queueFamilyIndex   = queueFamilyIdx,
        .queueCount         = 1,
        .pQueuePriorities   = queuePriority,
    };

    VkPhysicalDeviceFeatures allowedFeatures = {};
    vkGetPhysicalDeviceFeatures(phyDevice, &allowedFeatures);

    if (allowedFeatures.fillModeNonSolid != VK_TRUE) {
        printf("Error!: fillModeNonSolid is not supported on this device!\n");
    }

    VkPhysicalDeviceFeatures features = {};
    features.fillModeNonSolid = VK_TRUE;

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
        .pEnabledFeatures           = &features,
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

VkImageView Create2DImageView(
    const VkDevice  device,
    const VkFormat  format,
    const VkImage   image) {

    VkImageAspectFlags aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    if (format == VK_FORMAT_D32_SFLOAT || format == VK_FORMAT_D32_SFLOAT_S8_UINT) {
        aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    }

    VkImageViewCreateInfo createInfo = {
        .sType          = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .pNext          = nullptr,
        .flags          = 0,
        .image          = image,   // will be updated below
        .viewType       = VK_IMAGE_VIEW_TYPE_2D,
        .format         = format,
        .components     = {
            .r = VK_COMPONENT_SWIZZLE_IDENTITY,
            .g = VK_COMPONENT_SWIZZLE_IDENTITY,
            .b = VK_COMPONENT_SWIZZLE_IDENTITY,
            .a = VK_COMPONENT_SWIZZLE_IDENTITY },
        .subresourceRange = {
            .aspectMask     = aspectMask,
            .baseMipLevel   = 0,
            .levelCount     = 1,
            .baseArrayLayer = 0,
            .layerCount     = 1,
        }
    };

    VkImageView view = VK_NULL_HANDLE;
    VkResult result = vkCreateImageView(device, &createInfo, nullptr, &view);
    if (result != VK_SUCCESS) {
        // TODO: error report?
        return VK_NULL_HANDLE;
    }

    return view;
}


std::vector<VkImageView> Create2DImageViews(
    const VkDevice              device,
    const VkFormat              format,
    const std::vector<VkImage>& images) {

    std::vector<VkImageView> views(images.size(), VK_NULL_HANDLE);

    for (size_t idx = 0; idx < images.size(); idx++) {
        views[idx] = Create2DImageView(device, format, images[idx]);

        if (views[idx] == VK_NULL_HANDLE) {
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
    const VkFormat      depthFormat,
    VkRenderPass*       outRenderPass) {

    const VkAttachmentDescription attachments[] = {
        { // 0. color
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
        { // 1. depth
            .flags          = 0,
            .format         = depthFormat,
            .samples        = VK_SAMPLE_COUNT_1_BIT,
            .loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp        = VK_ATTACHMENT_STORE_OP_STORE,
            .stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED,
            .finalLayout    = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        },
    };

    VkAttachmentReference colorAttachmentRef = {
        .attachment = 0,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    };

    VkAttachmentReference depthAttachmentRef = {
        .attachment = 1,
        .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
    };

    const VkSubpassDescription subpass = {
        .flags                       = 0,
        .pipelineBindPoint          = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .inputAttachmentCount       = 0,
        .pInputAttachments          = NULL,
        .colorAttachmentCount       = 1,
        .pColorAttachments          = &colorAttachmentRef,
        .pResolveAttachments        = NULL,
        .pDepthStencilAttachment    = &depthAttachmentRef,
        .preserveAttachmentCount    = 0,
        .pPreserveAttachments       = NULL,
    };

    VkRenderPassCreateInfo createInfo = {
        .sType              = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .pNext              = nullptr,
        .flags              = 0,
        .attachmentCount    = std::size(attachments),
        .pAttachments       = attachments,
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
    const std::vector<VkImageView>& renderViews,
    const VkImageView               depthView) {

    std::vector<VkFramebuffer> framebuffers(renderViews.size(), VK_NULL_HANDLE);
    VkImageView attachments[] = {
        VK_NULL_HANDLE, // updated below
        depthView,
    };

    VkFramebufferCreateInfo createInfo = {
        .sType              = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .pNext              = NULL,
        .flags              = 0,
        .renderPass         = renderPass,
        .attachmentCount    = 2,
        .pAttachments       = attachments, // updated below
        .width              = width,
        .height             = height,
        .layers             = 1,
    };

    for (size_t idx = 0; idx < renderViews.size(); idx++) {
        attachments[0] = renderViews[idx];

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

VkPipelineLayout CreateEmptyPipelineLayout(
    const VkDevice          device,
    uint32_t                pushConstantSize = 0,
    VkDescriptorSetLayout   setLayout = nullptr) {

    VkPushConstantRange pushConstantRange = {
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
        .offset     = 0,
        .size       = pushConstantSize,
    };

    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {
        .sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .pNext                  = nullptr,
        .flags                  = 0,
        .setLayoutCount         = (uint32_t)(setLayout ? 1 : 0),
        .pSetLayouts            = &setLayout,
        .pushConstantRangeCount = (pushConstantSize > 0) ? 1u : 0u,
        .pPushConstantRanges    = &pushConstantRange,
    };

    VkPipelineLayout layout = VK_NULL_HANDLE;
    VkResult result = vkCreatePipelineLayout(device, &pipelineLayoutCreateInfo, nullptr, &layout);
    (void)result;

    return layout;
}


VkPipeline CreateSimpleVec3Pipeline(
    const VkDevice          device,
    const VkExtent2D        surfaceExtent,
    const VkRenderPass      renderPass,
    const VkPipelineLayout  pipelineLayout,
    const VkShaderModule    shaderVertex,
    const VkShaderModule    shaderFragment,
    const bool              depthTest = false,
    const bool              blendEnable = false) {

    // shader stages
    VkPipelineShaderStageCreateInfo shaders[] = {
        {
            .sType                  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .pNext                  = nullptr,
            .flags                  = 0,
            .stage                  = VK_SHADER_STAGE_VERTEX_BIT,
            .module                 = shaderVertex,
            .pName                  = "main",
            .pSpecializationInfo    = nullptr,
        },
        {
            .sType                  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .pNext                  = nullptr,
            .flags                  = 0,
            .stage                  = VK_SHADER_STAGE_FRAGMENT_BIT,
            .module                 = shaderFragment,
            .pName                  = "main",
            .pSpecializationInfo    = nullptr,
        }
    };

    // IMPORTANT!
    VkVertexInputBindingDescription vertexBinding = {
        // binding position in Vulkan API, maches the binding in VkVertexInputAttributeDescription
        .binding   = 0,
        .stride    = sizeof(float) * 5,           // step by "vec3" elements in the attached buffer
        .inputRate = VK_VERTEX_INPUT_RATE_VERTEX, // step "stride" bytes each for vertex in the buffer
    };

    // IMPORTANT!
    VkVertexInputAttributeDescription vertexAttribute = {
        .location = 0,                          // layout location=0 in shader
        .binding  = 0,                          // binding position in Vulkan API
        .format   = VK_FORMAT_R32G32B32_SFLOAT, // use "vec3" values from the buffer
        .offset   = 0,                          // use buffer from the 0 byte
    };

    // IMPORTANT! related buffer(s) must be bound before draw via vkCmdBindVertexBuffers
    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {
        .sType                              = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .pNext                              = 0,
        .flags                              = 0,
        .vertexBindingDescriptionCount      = 1u,
        .pVertexBindingDescriptions         = &vertexBinding,
        .vertexAttributeDescriptionCount    = 1u,
        .pVertexAttributeDescriptions       = &vertexAttribute,
    };

    // input assembly
    VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo = {
        .sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .pNext                  = nullptr,
        .flags                  = 0,
        .topology               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .primitiveRestartEnable = VK_FALSE,
    };

    // viewport info
    VkViewport viewport = {
        .x          = 0,
        .y          = 0,
        .width      = float(surfaceExtent.width),
        .height     = float(surfaceExtent.height),
        .minDepth   = 0.0f,
        .maxDepth   = 1.0f,
    };

    VkRect2D scissor {
        .offset = { 0, 0 },
        .extent = surfaceExtent,
    };

    VkPipelineViewportStateCreateInfo viewportInfo = {
        .sType          = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .pNext          = nullptr,
        .flags          = 0,
        .viewportCount  = 1,
        .pViewports     = &viewport,
        .scissorCount   = 1,
        .pScissors      = &scissor,
    };

    // rasterization info
    VkPipelineRasterizationStateCreateInfo rasterizationInfo = {
        .sType       = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .pNext                   = nullptr,
        .flags                   = 0,
        .depthClampEnable        = VK_FALSE,
        .rasterizerDiscardEnable = VK_FALSE,
        .polygonMode             = VK_POLYGON_MODE_FILL,
        .cullMode                = VK_CULL_MODE_NONE, //VK_CULL_MODE_FRONT_BIT,
        .frontFace               = VK_FRONT_FACE_CLOCKWISE,
        .depthBiasEnable         = VK_FALSE,
        .depthBiasConstantFactor = 0.0f, // Disabled
        .depthBiasClamp          = 0.0f, // Disabled
        .depthBiasSlopeFactor    = 0.0f, // Disabled
        .lineWidth               = 1.0f,
    };

    // multisample
    VkPipelineMultisampleStateCreateInfo multisampleInfo = {
        .sType                  = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .pNext                  = nullptr,
        .flags                  = 0,
        .rasterizationSamples   = VK_SAMPLE_COUNT_1_BIT,
        .sampleShadingEnable    = VK_FALSE,
        .minSampleShading       = 0.0f,
        .pSampleMask            = nullptr,
        .alphaToCoverageEnable  = VK_FALSE,
        .alphaToOneEnable       = VK_FALSE,
    };

    // depth stencil
    // "empty" stencil Op state
    VkStencilOpState emptyStencilOp = {
        .failOp      = VK_STENCIL_OP_KEEP,
        .passOp      = VK_STENCIL_OP_KEEP,
        .depthFailOp = VK_STENCIL_OP_KEEP,
        .compareOp   = VK_COMPARE_OP_NEVER,
        .compareMask = 0,
        .writeMask   = 0,
        .reference   = 0,
    };

    VkPipelineDepthStencilStateCreateInfo depthStencilInfo = {
        .sType                 = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .pNext                 = nullptr,
        .flags                 = 0,
        .depthTestEnable       = depthTest ? VK_TRUE : VK_FALSE,
        .depthWriteEnable      = VK_TRUE,
        .depthCompareOp        = VK_COMPARE_OP_LESS,
        .depthBoundsTestEnable = VK_FALSE,
        .stencilTestEnable     = VK_FALSE,
        .front                 = emptyStencilOp,
        .back                  = emptyStencilOp,
        .minDepthBounds        = 0.0f,
        .maxDepthBounds        = 1.0f,
    };

    // color blend
    VkPipelineColorBlendAttachmentState blendAttachment = {
        .blendEnable         = blendEnable ? VK_TRUE : VK_FALSE,
        // as blend is disabled fill these with default values,
        .srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
        .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA, //VK_BLEND_FACTOR_ZERO, //ONE, //DST_COLOR,
        .colorBlendOp        = VK_BLEND_OP_ADD,
        .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE, //SRC_ALPHA, //SRC_ALPHA, //VK_BLEND_FACTOR_SRC_ALPHA,
        .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO, //VK_BLEND_FACTOR_DST_ALPHA, //ZERO,
        .alphaBlendOp        = VK_BLEND_OP_ADD,
        // Important!
        .colorWriteMask      = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
    };

    VkPipelineColorBlendStateCreateInfo colorBlendInfo = {
        .sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .pNext           = nullptr,
        .flags           = 0,
        .logicOpEnable   = VK_FALSE, //FALSE,
        .logicOp         = VK_LOGIC_OP_CLEAR, // Disabled
        // Important!
        .attachmentCount = 1,
        .pAttachments    = &blendAttachment,
        .blendConstants  = { 1.0f, 1.0f, 1.0f, 1.0f }, // Ignored
    };


    // pipeline create
    VkGraphicsPipelineCreateInfo pipelineCreateInfo = {
        .sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pNext               = nullptr,
        .flags               = 0,
        .stageCount          = 2,
        .pStages             = shaders,
        .pVertexInputState   = &vertexInputInfo,
        .pInputAssemblyState = &inputAssemblyInfo,
        .pTessellationState  = nullptr,
        .pViewportState      = &viewportInfo,
        .pRasterizationState = &rasterizationInfo,
        .pMultisampleState   = &multisampleInfo,
        .pDepthStencilState  = &depthStencilInfo,
        .pColorBlendState    = &colorBlendInfo,
        .pDynamicState       = nullptr,
        .layout              = pipelineLayout,
        .renderPass          = renderPass,
        .subpass             = 0,
        .basePipelineHandle  = VK_NULL_HANDLE,
        .basePipelineIndex   = 0,
    };

    VkPipeline pipeline = VK_NULL_HANDLE;
    VkResult result = vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &pipeline);
    (void)result;

    return pipeline;
}

uint32_t FindMemoryTypeIndex(const VkPhysicalDevice phyDevice, const VkMemoryRequirements& requirements, VkMemoryPropertyFlags flags) {
    VkPhysicalDeviceMemoryProperties memoryProperties = {};
    vkGetPhysicalDeviceMemoryProperties(phyDevice, &memoryProperties);

    for (uint32_t idx = 0; idx < memoryProperties.memoryTypeCount; idx++) {
        if (requirements.memoryTypeBits & (1 << idx)) {
            const VkMemoryType& memoryType = memoryProperties.memoryTypes[idx];
            // TODO: add size check?

            if (memoryType.propertyFlags & flags) {
                return idx;
            }
        }
    }

    return (uint32_t)-1;
}

struct ImageInfo {
    VkFormat        format;
    uint32_t        width;
    uint32_t        height;
    VkDeviceSize    size;
    VkImage         image;
    VkDeviceMemory  memory;
};

ImageInfo Create2DImage(
    const VkPhysicalDevice  phyDevice,
    const VkDevice          device,
    uint32_t                width,
    uint32_t                height,
    VkFormat                format,
    VkImageUsageFlags       usage) {

    VkImageCreateInfo createInfo = {
        .sType                  = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .pNext                  = nullptr,
        .flags                  = 0,
        .imageType              = VK_IMAGE_TYPE_2D,
        .format                 = format,
        .extent                 = { width, height, 1 },
        .mipLevels              = 1,
        .arrayLayers            = 1,
        .samples                = VK_SAMPLE_COUNT_1_BIT,
        .tiling                 = VK_IMAGE_TILING_OPTIMAL,
        .usage                  = usage,
        .sharingMode            = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount  = 0,
        .pQueueFamilyIndices    = nullptr,
        .initialLayout          = VK_IMAGE_LAYOUT_UNDEFINED,
    };

    ImageInfo result = { format, width, height, 0, VK_NULL_HANDLE, VK_NULL_HANDLE };
    VkResult createResult = vkCreateImage(device, &createInfo, nullptr, &result.image);
    (void)createResult; // TODO: error check

    // Memory
    VkMemoryRequirements requirements = {};
    vkGetImageMemoryRequirements(device, result.image, &requirements);

    const uint32_t memoryTypeIdx = FindMemoryTypeIndex(phyDevice, requirements, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    // TODO: check for error

    VkMemoryAllocateInfo allocInfo = {
        .sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .pNext           = 0,
        .allocationSize  = requirements.size,
        .memoryTypeIndex = memoryTypeIdx,
    };

    vkAllocateMemory(device, &allocInfo, nullptr, &result.memory);

    vkBindImageMemory(device, result.image, result.memory, 0);


    return result;
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

#if (GLFW_VERSION_MAJOR >= 3) && (GLFW_VERSION_MINOR >= 4)
    if (glfwPlatformSupported(GLFW_PLATFORM_X11)) {
        glfwInitHint(GLFW_PLATFORM, GLFW_PLATFORM_X11);
    }
#endif

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
    InitializeDebugCallback(instance, static_cast<PFN_vkDebugUtilsMessengerCallbackEXT>(DebugCallback), nullptr, &debugMessenger);

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

    VkFormat depthFormat = VK_FORMAT_D32_SFLOAT_S8_UINT;
    ImageInfo depthInfo = Create2DImage(phyDevice, device, windowWidth, windowHeight, depthFormat, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);
    VkImageView depthView = Create2DImageView(device, depthFormat, depthInfo.image);

    VkRenderPass renderPass = VK_NULL_HANDLE;
    CreateSimpleRenderPass(device, surfaceInfo.format, depthFormat, &renderPass); // TODO: check result

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

    std::vector<VkFramebuffer> framebuffers = CreateSimpleFramebuffers(device, renderPass, windowWidth, windowHeight, swapchainViews, depthView);

    DescriptorMgmt descriptors;
    descriptors.SetDescriptor(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1);
    descriptors.CreateLayout(device);
    descriptors.CreatePool(device);

    descriptors.CreateDescriptorSets(device, 1);

    struct UBO {
        glm::vec4 colors[3];
    } ubo = {
        {
            {1.0f, 0.0f, 0.0f, 0.0f},
            {0.0f, 1.0f, 0.0f, 0.0f},
            {0.0f, 0.0f, 1.0f, 0.0f},
        }
    };

    BufferInfo colorInfo = BufferInfo::Create(phyDevice, device, sizeof(ubo), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
    colorInfo.Update(device, &ubo, sizeof(ubo));

    DescriptorSetMgmt &colorSet = descriptors.Set(0);
    colorSet.SetBuffer(0, colorInfo.buffer);
    colorSet.Update(device);

    // Create a buffer and upload the cube vertices
    float cubeVertices[] = {
        #include "05_cube_vertices.inc"
    };

    BufferInfo cubeVertexInfo = BufferInfo::Create(phyDevice, device, sizeof(cubeVertices), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    cubeVertexInfo.Update(device, cubeVertices, sizeof(cubeVertices));

    // Fill the MVP matrix with identity
    float MVP[4][4] = {
        { 1.0f, 0.0f, 0.0f, 0.0f },
        { 0.0f, 1.0f, 0.0f, 0.0f },
        { 0.0f, 0.0f, 1.0f, 0.0f },
        { 0.0f, 0.0f, 0.0f, 1.0f },
    };

    VkExtent2D surfaceExtent = { (uint32_t)windowWidth, (uint32_t)windowHeight };
    VkPipelineLayout trianglePipelineLayout = CreateEmptyPipelineLayout(device, sizeof(MVP), descriptors.Layout());

    VkShaderModule shaderVertex     = CreateShaderModule(device, SPV_triangle_in_vert, sizeof(SPV_triangle_in_vert));
    VkShaderModule shaderFragment   = CreateShaderModule(device, SPV_triangle_in_frag, sizeof(SPV_triangle_in_frag));

    VkPipeline cubeNoDepthPipeline = CreateSimpleVec3Pipeline(device, surfaceExtent, renderPass, trianglePipelineLayout, shaderVertex, shaderFragment, false);
    VkPipeline cubeDepthPipeline = CreateSimpleVec3Pipeline(device, surfaceExtent, renderPass, trianglePipelineLayout, shaderVertex, shaderFragment, true);

    // Destroy shader modules, pipeline already created
    vkDestroyShaderModule(device, shaderVertex, nullptr);
    vkDestroyShaderModule(device, shaderFragment, nullptr);


    Grid grid(4.0f, 4.0f, 2);
    grid.dump();
    grid.BuildPipeline(device, surfaceExtent, renderPass, trianglePipelineLayout);
    grid.BuildVertices(phyDevice, device);

    VkFence imageFence              = CreateFence(device);
    VkSemaphore presentSemaphore    = CreateSemaphore(device);

    glfwShowWindow(window);


    glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)windowWidth / (float)windowHeight, 0.1f, 100.0f);

    projection[1][1] *= -1;

    struct {
        int32_t x;
        int32_t y;
        int32_t z;
    } rotation = { 20, 10, 30 };

    bool rotationAutoInc = true;
    bool useDepth = true;

    glm::vec3 cameraPos   = glm::vec3(0.0f, 1.0f,  3.0f);
    glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
    glm::vec3 cameraUp    = glm::vec3(0.0f, 1.0f,  0.0f);

    float yaw   = -90.0f;   // yaw is initialized to -90.0 degrees since a yaw of 0.0 results in a direction vector pointing to the right so we initially rotate a bit to the left.
    float pitch =  -10.0f;
    float lastX =  800.0f / 2.0;
    float lastY =  600.0 / 2.0;

    // tell GLFW to capture our mouse
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        {
            float cameraSpeed = static_cast<float>(2.5 * 0.05); //deltaTime);

            if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
                cameraPos += cameraSpeed * cameraFront;
            }
            if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
                cameraPos -= cameraSpeed * cameraFront;
            }
            if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
                cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
            }
            if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
                cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
            }

            double xposIn, yposIn;
            glfwGetCursorPos(window, &xposIn, &yposIn);

            float xpos = static_cast<float>(xposIn);
            float ypos = static_cast<float>(yposIn);
            static bool firstMouse = true;

            if (firstMouse) {
                lastX = xpos;
                lastY = ypos;
                firstMouse = false;
            }

            float xoffset = xpos - lastX;
            float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top
            lastX = xpos;
            lastY = ypos;

            float sensitivity = 0.1f; // change this value to your liking
            xoffset *= sensitivity;
            yoffset *= sensitivity;

            yaw += xoffset;
            pitch += yoffset;

            // make sure that when pitch is out of bounds, screen doesn't get flipped
            if (pitch > 89.0f) {
                pitch = 89.0f;
            }
            if (pitch < -89.0f) {
                pitch = -89.0f;
            }

            glm::vec3 front;
            front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
            front.y = sin(glm::radians(pitch));
            front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
            cameraFront = glm::normalize(front);
        }

        {
            ImGui_ImplVulkan_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

            //ImGui::ShowDemoWindow();

            ImGui::Begin("Info");

            ImGui::Checkbox("Use depth testing", &useDepth);
            ImGui::Checkbox("Use auto rotation", &rotationAutoInc);

            if (rotationAutoInc) {
                rotation.x = (rotation.x + 1) % 360;
                rotation.y = (rotation.y + 1) % 360;
                rotation.z = (rotation.z + 1) % 360;
            }

            ImGui::SliderInt("Rotation X", &rotation.x, 0, 360);
            ImGui::SliderInt("Rotation Y", &rotation.y, 0, 360);
            ImGui::SliderInt("Rotation Z", &rotation.z, 0, 360);

            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);

            ImGui::InputFloat3("Camera Positon", (float*)&cameraPos);
            float cameraRotation[2] = { pitch, yaw };
            ImGui::InputFloat2("Camera Rotation", cameraRotation);
            ImGui::End();

            ImGui::Render();
        }

        vkResetFences(device, 1, &imageFence);

        uint32_t swapchainIdx = -1;
        vkAcquireNextImageKHR(device, swapchain, 1e9 * 2, VK_NULL_HANDLE, imageFence, &swapchainIdx);
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

            VkClearValue clears[2];
            clears[0].color = { { 0.0f, 0.0f, 0.0f, 1.0f } };
            clears[1].depthStencil = {1.0f, 0};

            VkRenderPassBeginInfo renderPassInfo = {
                .sType          = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
                .pNext          = nullptr,
                .renderPass     = renderPass,
                .framebuffer    = framebuffers[swapchainIdx],
                .renderArea     = {
                    .offset = { 0, 0 },
                    .extent = { (uint32_t)windowWidth, (uint32_t)windowHeight },
                },
                .clearValueCount = 2,
                .pClearValues = clears,
            };

            glm::mat4 view = glm::mat4(1.0f);
            /*
            view = glm::lookAt(glm::vec3(0.0f, 1.0f, 3.0f), // camera position
                               glm::vec3(0.0f, 0.0f, 0.0f), // camera target
                                glm::vec3(0.0f, 1.0f, 0.0f)); // up direction
            */
            view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);

            vkCmdBeginRenderPass(cmdBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

            {
                glm::mat4 model =
                    glm::rotate(glm::mat4(1.0f), glm::radians((float)rotation.x), glm::vec3(1.0f, 0.0f, 0.0f))
                    * glm::rotate(glm::mat4(1.0f), glm::radians((float)rotation.y), glm::vec3(0.0f, 1.0f, 0.0f))
                    * glm::rotate(glm::mat4(1.0f), glm::radians((float)rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));

                glm::mat4 mvp = projection * view * model; //model * view * projection;

                // Cube bind and draw
                VkPipeline cubePipeline = useDepth ? cubeDepthPipeline : cubeNoDepthPipeline;
                vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, cubePipeline);
                vkCmdPushConstants(cmdBuffer, trianglePipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(MVP), &mvp);

                vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, trianglePipelineLayout, 0, 1, &colorSet.Get(), 0, nullptr);

                VkDeviceSize offsets[] = { 0 };
                vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &cubeVertexInfo.buffer, offsets);
                vkCmdDraw(cmdBuffer, 36, 1, 0, 0);
            }

            {
                // draw the grid
                glm::mat4 model =
                    glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));

                glm::mat4 mvp = projection * view * model;

                vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, grid.pipeline);
                vkCmdPushConstants(cmdBuffer, trianglePipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(MVP), &mvp);

                vkCmdBindIndexBuffer(cmdBuffer, grid.indexInfo.buffer, 0, VK_INDEX_TYPE_UINT32);

                VkDeviceSize offsets[] = { 0 };
                vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &grid.vertexInfo.buffer, offsets);

                vkCmdDrawIndexed(cmdBuffer, grid.indices.size(), 1, 0, 0, 0);
            }


            {
                // IMGUI
                ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmdBuffer);
            }

            vkCmdEndRenderPass(cmdBuffer);

            VkResult endResult = vkEndCommandBuffer(cmdBuffer);
            (void)endResult;
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

    vkDestroyPipeline(device, cubeDepthPipeline, nullptr);
    vkDestroyPipeline(device, cubeNoDepthPipeline, nullptr);
    vkDestroyPipelineLayout(device, trianglePipelineLayout, nullptr);

    grid.Destroy(device);

    cubeVertexInfo.Destroy(device);

    DestroyFramebuffers(device, framebuffers);
    vkDestroyRenderPass(device, renderPass, nullptr);

    vkDestroyImage(device, depthInfo.image, nullptr);
    vkDestroyImageView(device, depthView, nullptr);
    vkFreeMemory(device, depthInfo.memory, nullptr);

    vkDestroyDescriptorPool(device, descPool, nullptr);
    vkDestroyCommandPool(device, cmdPool, nullptr);

    colorInfo.Destroy(device);
    descriptors.Destroy(device);

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
