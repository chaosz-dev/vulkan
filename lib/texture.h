#pragma once

#include <string>

#include <vulkan/vulkan_core.h>

VkImageView Create2DImageView(
    const VkDevice  device,
    const VkFormat  format,
    const VkImage   image);


struct BufferInfo;

class Texture {
public:
    static Texture *LoadFromFile(
        const VkPhysicalDevice  phyDevice,
        const VkDevice          device,
        const VkQueue           queue,
        const VkCommandPool     cmdPool,
        const std::string&      path,
        const VkFormat          format,
        VkImageUsageFlags       usage);

/*
    static Texture *LoadFromData(
        const VkPhysicalDevice  phyDevice,
        const VkDevice          device,
        const VkQueue           queue,
        const VkCommandPool     cmdPool,
        const std::string&      path,
        const VkFormat          format,
        VkImageUsageFlags       usage);
*/

    static Texture Create2D(
        const VkPhysicalDevice  phyDevice,
        const VkDevice          device,
        const VkFormat          format,
        VkExtent2D              extent,
        VkImageUsageFlags       usage,
        VkSampleCountFlagBits   msaaSamples = VK_SAMPLE_COUNT_1_BIT);

    VkImage image() const { return m_image; }
    VkImageView view() const { return m_view; }
    VkSampler sampler() const { return m_sampler; }

    bool UploadFromBuffer(
        const VkDevice      device,
        const VkQueue       queue,
        const VkCommandPool cmdPool,
        const VkBuffer&     rawBuffer);

    bool Create2DSampler(const VkDevice device);

    void Destroy(const VkDevice device);

    bool IsValid() const { return m_format != VK_FORMAT_UNDEFINED; }

    uint32_t Width() const { return m_width; }
    uint32_t Height() const { return m_height; }

    VkExtent2D Extent2D() const { return { m_width, m_height }; }

    Texture()
        : Texture(VK_FORMAT_UNDEFINED, 0, 0)
    {}

private:
    Texture(VkFormat format, uint32_t width, uint32_t height)
        : m_format(format)
        , m_width(width)
        , m_height(height)
    {}

    VkResult CreateImage(
        const VkPhysicalDevice  phyDevice,
        const VkDevice          device,
        VkImageUsageFlags       usage,
        VkSampleCountFlagBits   msaaSamples = VK_SAMPLE_COUNT_1_BIT);

    bool InitFromBuffer(
        const VkPhysicalDevice  phyDevice,
        const VkDevice          device,
        const VkQueue           queue,
        const VkCommandPool     cmdPool,
        VkImageUsageFlags       usage,
        const VkBuffer          buffer);


    VkFormat m_format;
    uint32_t m_width;
    uint32_t m_height;

    VkImage m_image;
    VkDeviceMemory m_memory;

    VkImageView m_view;
    VkSampler m_sampler;
};
