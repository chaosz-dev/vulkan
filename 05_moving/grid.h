#pragma once

#include <cstdint>
#include <vector>

#include <vulkan/vulkan_core.h>

#include "buffer.h"

struct Grid {
    std::vector<float>      vertices;
    uint32_t                vertexCount;
    std::vector<uint32_t>   indices;
    VkPipeline              pipeline;

    BufferInfo              vertexInfo;
    BufferInfo              indexInfo;

    Grid(float width, float height, uint32_t count);
    void dump();

    void BuildPipeline(
        const VkDevice          device,
        const VkExtent2D        surfaceExtent,
        const VkRenderPass      renderPass,
        const VkPipelineLayout  pipelineLayout);

    void BuildVertices(
        const VkPhysicalDevice  phyDevice,
        const VkDevice          device);

    void Destroy(const VkDevice device);

    size_t vertexSize() const { return vertices.size() * sizeof(float); }
    size_t indexSize() const { return indices.size() * sizeof(uint32_t); }
};
