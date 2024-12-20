#pragma once

#include <vulkan/vulkan_core.h>

class LightningPass {
public:

    void BuildPipeline(
        const VkDevice          device,
        const VkExtent2D        surfaceExtent,
        const VkRenderPass      renderPass,
        const VkPipelineLayout  pipelineLayout,
        const VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_1_BIT);

    void Destroy(const VkDevice device);

    VkPipeline SimplePipeline() const { return m_simplePipeline; }
    VkPipeline ShadowMapPipeline() const { return m_shadowMapPipeline; }

private:

    VkPipeline m_simplePipeline     = VK_NULL_HANDLE;
    VkPipeline m_shadowMapPipeline  = VK_NULL_HANDLE;
};
