#include "buffer.h"

#include <cstring>

static uint32_t FindMemoryTypeIndex(const VkPhysicalDevice phyDevice, const VkMemoryRequirements& requirements, VkMemoryPropertyFlags flags) {
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

BufferInfo BufferInfo::Create(
    const VkPhysicalDevice  phyDevice,
    const VkDevice          device,
    VkDeviceSize            size,
    VkBufferUsageFlags      usageFlags) {

    VkBufferCreateInfo createInfo = {
        .sType                 = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext                 = nullptr,
        .flags                 = 0,
        .size                  = size,
        .usage                 = usageFlags,
        .sharingMode           = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices   = nullptr,
    };

    BufferInfo result = { };

    vkCreateBuffer(device, &createInfo, nullptr, &result.buffer);
    // TODO: error check

    VkMemoryRequirements requirements = {};
    vkGetBufferMemoryRequirements(device, result.buffer, &requirements);

    const uint32_t memoryTypeIdx = FindMemoryTypeIndex(phyDevice, requirements, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
    // TODO: check for error

    VkMemoryAllocateInfo allocInfo = {
        .sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .pNext           = 0,
        .allocationSize  = requirements.size,
        .memoryTypeIndex = memoryTypeIdx,
    };

    vkAllocateMemory(device, &allocInfo, nullptr, &result.memory);

    vkBindBufferMemory(device, result.buffer, result.memory, 0);

    return result;
}

void* BufferInfo::Map(const VkDevice device) {
    void* GPUPtr = nullptr;
    vkMapMemory(device, memory, 0, VK_WHOLE_SIZE, 0, &GPUPtr);

    return GPUPtr;
}

void BufferInfo::Unmap(const VkDevice device) {
    vkUnmapMemory(device, memory);
}

void BufferInfo::Update(const VkDevice device, const void* inputPtr, size_t size) {
    void* ptr = Map(device);

    memcpy(ptr, inputPtr, size);

    Unmap(device);
}

void BufferInfo::Destroy(const VkDevice device) {
    vkDestroyBuffer(device, buffer, nullptr);
    vkFreeMemory(device, memory, nullptr);
}
