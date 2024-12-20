#pragma once

#include <vulkan/vulkan_core.h>

struct BufferInfo {
    VkDeviceSize   size;
    VkBuffer       buffer;
    VkDeviceMemory memory;

    static BufferInfo Create(const VkPhysicalDevice phyDevice, const VkDevice device, VkDeviceSize size, VkBufferUsageFlags usageFlags);

    void* Map(const VkDevice device);
    void Unmap(const VkDevice device);

    void Update(const VkDevice device, const void* inputPtr, size_t size);

    void Destroy(const VkDevice device);
};
