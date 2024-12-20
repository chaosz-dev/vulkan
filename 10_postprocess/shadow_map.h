#pragma once

#include <vulkan/vulkan_core.h>

#include "texture.h"

class ShadowMap {
public:
    bool Build(const VkPhysicalDevice phyDevice,
               const VkDevice device,
               uint32_t size);

    bool BuildPipeline(const VkDevice device, const VkPipelineLayout pipelineLayout);

    void Destroy(const VkDevice device);

    VkExtent2D Extent() const { return m_extent; }
    uint32_t Width() const { return m_extent.width; }
    uint32_t Height() const{ return m_extent.height; }

    VkViewport& Viewport() const {
        static VkViewport viewport = {
                .x          = 0,
                .y          = 0,
                .width      = float(m_extent.width),
                .height     = float(m_extent.height),
                .minDepth   = 0.0f,
                .maxDepth   = 1.0f,
            };
        return viewport;
    }

    VkPipeline Pipeline() const { return m_pipeline; }
    Texture& Depth() { return m_shadowDepth; }

    void BeginPass(const VkCommandBuffer cmdBuffer);

private:
    bool BuildRenderpass(const VkDevice device);
    bool BuildFBO(const VkDevice device);

    const VkFormat  m_depthFormat   = VK_FORMAT_D32_SFLOAT_S8_UINT;

    VkExtent2D      m_extent        = { 0, 0 };
    VkPipeline      m_pipeline      = VK_NULL_HANDLE;
    VkRenderPass    m_renderPass    = VK_NULL_HANDLE;
    VkFramebuffer   m_framebuffer   = VK_NULL_HANDLE;

    Texture         m_shadowDepth;
};
