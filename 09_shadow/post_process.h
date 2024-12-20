#pragma once

#include <vulkan/vulkan_core.h>

#include "descriptors.h"
#include "texture.h"

class PostProcessPass {
public:

    void BuildPipeline(
        const VkDevice          device,
        const VkExtent2D        surfaceExtent,
        const VkRenderPass      renderPass);

    void BindInputImage(const VkDevice device, const Texture& texture);

    void Destroy(const VkDevice device);

    void BindPipeline(VkCommandBuffer cmdBuffer);
    void Draw(VkCommandBuffer cmdBuffer);

    VkPipeline          Pipeline() const { return m_pipeline; }
    VkPipelineLayout    PipelineLayout() const { return m_pipelineLayout; }
    VkDescriptorSet     DescSet() { return m_descMgmt.Set(0).Get(); }

private:
    DescriptorMgmt      m_descMgmt          = {};

    VkPipelineLayout    m_pipelineLayout    = VK_NULL_HANDLE;
    VkPipeline          m_pipeline          = VK_NULL_HANDLE;
};
