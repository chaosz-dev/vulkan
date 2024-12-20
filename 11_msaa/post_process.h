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
    void BindMSInputImage(const VkDevice device, const Texture& texture);

    void UseMode(uint32_t mode) { m_mode = mode; }
    void UseMSAAInput(bool useMsaa) { m_useMsaa = useMsaa; }
    void UseMSAASamples(uint32_t sampleCount) { m_useMsaaSamples = sampleCount; }

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

    uint32_t            m_mode              = 0;
    bool                m_useMsaa           = false;
    uint32_t            m_useMsaaSamples    = 1;
};
