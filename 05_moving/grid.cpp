#include "grid.h"

#include <cstdio>

#include <vulkan/vulkan_core.h>

#include "shader_tooling.h"

namespace {
#include "grid.vert_include.h"
#include "grid.frag_include.h"
}

static std::vector<float> buildGrid(float width, float height, uint32_t count) {
    // Output format: { x, y, z, u, v }
    std::vector<float> result;

    float halfWidth = width / 2.0f;
    float halfHeight = height / 2.0f;

    for (uint32_t y = 0; y <= count; y++) {
        for (uint32_t x = 0; x <= count; x++) {
            float vertex[] = {
                (float)x / count * width - halfWidth,
                (float)y / count * height - halfHeight,
                0.0f,
                (float)x / count,
                (float)y / count
            };

            result.insert(result.end(), vertex, vertex + 5);
        }
    }

    return result;
}


static std::vector<uint32_t> buildIndexList(uint32_t splitCount) {

    std::vector<uint32_t> indexList;

    for (uint32_t y = 0; y < splitCount; y++) {
        for (uint32_t x = 0; x < splitCount; x++) {
            uint32_t row = y * (splitCount + 1);
            uint32_t rowNext = (y + 1) * (splitCount + 1);

            uint32_t triangleIndices[] = {
                // triangle 1
                row + x,  row + x + 1, rowNext + x + 1,

                // triangle 2
                row + x, rowNext + x + 1, rowNext + x,
            };

            indexList.insert(indexList.end(), triangleIndices, triangleIndices + 6);
        }
    }

    return indexList;
}

static VkPipeline CreatePipeline(
    const VkDevice          device,
    const VkExtent2D        surfaceExtent,
    const VkRenderPass      renderPass,
    const VkPipelineLayout  pipelineLayout,
    const VkShaderModule    shaderVertex,
    const VkShaderModule    shaderFragment) {

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
        .polygonMode             = VK_POLYGON_MODE_LINE, //FILL,
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
        .depthTestEnable       = VK_TRUE,
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
        .blendEnable         = VK_TRUE,
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


Grid::Grid(float width, float height, uint32_t count)
    : vertices(buildGrid(width, height, count))
    , vertexCount(vertices.size() / 5)
    , indices(buildIndexList(count)) {
}

void Grid::dump() {
    printf("Vertices: (count: %u)\n" , vertexCount);
    for (size_t idx = 0; idx < vertices.size(); idx++) {
        printf("%-3.2f, ", vertices[idx]);
        if ((idx + 1) % 5 == 0) {
            printf("\n");
        }
    }

    printf("Indices: (count: %lu)\n", indices.size());
    for (size_t idx = 0; idx < indices.size(); idx++) {
        printf("%3u, ", indices[idx]);
        if ((idx + 1) % 3 == 0) {
            printf("\n");
        }
    }
}

void Grid::BuildPipeline(
    const VkDevice          device,
    const VkExtent2D        surfaceExtent,
    const VkRenderPass      renderPass,
    const VkPipelineLayout  pipelineLayout) {

    VkShaderModule gridShaders[] = {
        CreateShaderModule(device, SPV_grid_vert, sizeof(SPV_grid_vert)),
        CreateShaderModule(device, SPV_grid_frag, sizeof(SPV_grid_frag)),
    };

    pipeline = CreatePipeline(device, surfaceExtent, renderPass, pipelineLayout, gridShaders[0], gridShaders[1]);

    vkDestroyShaderModule(device, gridShaders[0], nullptr);
    vkDestroyShaderModule(device, gridShaders[1], nullptr);
}

void Grid::BuildVertices(
    const VkPhysicalDevice  phyDevice,
    const VkDevice          device) {

    vertexInfo = BufferInfo::Create(phyDevice, device, vertexSize(), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    vertexInfo.Update(device, vertices.data(), vertexSize());

    indexInfo = BufferInfo::Create(phyDevice, device, indexSize(), VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
    indexInfo.Update(device, indices.data(), indexSize());
}

void Grid::Destroy(const VkDevice device) {
    vkDestroyPipeline(device, pipeline, nullptr);

    vertexInfo.Destroy(device);
    indexInfo.Destroy(device);
}
