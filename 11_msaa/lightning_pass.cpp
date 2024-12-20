#include "lightning_pass.h"

#include "shader_tooling.h"

#include "debug.h"

namespace {
#include "lightning_simple.vert_include.h"
#include "lightning_simple.frag_include.h"

#include "lightning_shadowmap.vert_include.h"
#include "lightning_shadowmap.frag_include.h"
}


static VkPipeline CreatePipeline(
    const VkDevice              device,
    const VkExtent2D            surfaceExtent,
    const VkRenderPass          renderPass,
    const VkPipelineLayout      pipelineLayout,
    const VkShaderModule        shaderVertex,
    const VkShaderModule        shaderFragment,
    const VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_1_BIT) {

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
        .stride    = sizeof(float) * (3 + 2 + 3), // step by "vec3" elements in the attached buffer
        .inputRate = VK_VERTEX_INPUT_RATE_VERTEX, // step "stride" bytes each for vertex in the buffer
    };

    // IMPORTANT!
    VkVertexInputAttributeDescription vertexAttributes[3] = {
        { // position
            .location = 0,                          // layout location=0 in shader
            .binding  = 0,                          // binding position in Vulkan API
            .format   = VK_FORMAT_R32G32B32_SFLOAT, // use "vec3" values from the buffer
            .offset   = 0,                          // use buffer from the 0 byte
        },
        { // uv
            .location = 1,                          // layout location=1 in shader
            .binding  = 0,                          // binding position in Vulkan API
            .format   = VK_FORMAT_R32G32_SFLOAT, // use "vec2" values from the buffer
            .offset   = sizeof(float) * 3,          // use buffer from the 3*float
        },
        { // normal
            .location = 2,                          // layout location=2 in shader
            .binding  = 0,                          // binding position in Vulkan API
            .format   = VK_FORMAT_R32G32B32_SFLOAT, // use "vec2" values from the buffer
            .offset   = sizeof(float) * (3 + 2),    // use buffer from the 3*float
        }
    };

    // IMPORTANT! related buffer(s) must be bound before draw via vkCmdBindVertexBuffers
    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {
        .sType                              = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .pNext                              = 0,
        .flags                              = 0,
        .vertexBindingDescriptionCount      = 1u,
        .pVertexBindingDescriptions         = &vertexBinding,
        .vertexAttributeDescriptionCount    = 3u,
        .pVertexAttributeDescriptions       = vertexAttributes,
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
        .rasterizationSamples   = msaaSamples,
        .sampleShadingEnable    = VK_FALSE,
        .minSampleShading       = 0.0f,
        .pSampleMask            = nullptr,
        .alphaToCoverageEnable  = VK_FALSE,
        .alphaToOneEnable       = VK_FALSE,
    };

    // depth stencil
    // "empty" stencil Op state
    VkStencilOpState emptyStencilOp = { };

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
        .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
        .colorBlendOp        = VK_BLEND_OP_ADD,
        .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
        .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
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

    VkDynamicState dynamicStates[] = {
        VK_DYNAMIC_STATE_VIEWPORT,
    };

    VkPipelineDynamicStateCreateInfo dynamicStateInfo = {
        .sType              = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .pNext              = nullptr,
        .flags              = 0,
        .dynamicStateCount  = 1,
        .pDynamicStates     = dynamicStates,
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
        .pDynamicState       = &dynamicStateInfo,
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

void LightningPass::BuildPipeline(
    const VkDevice          device,
    const VkExtent2D        surfaceExtent,
    const VkRenderPass      renderPass,
    const VkPipelineLayout  pipelineLayout,
    const VkSampleCountFlagBits msaaSamples) {

    // Simple lightning
    {
        VkShaderModule gridShaders[] = {
            CreateShaderModule(device, SPV_lightning_simple_vert, sizeof(SPV_lightning_simple_vert)),
            CreateShaderModule(device, SPV_lightning_simple_frag, sizeof(SPV_lightning_simple_frag)),
        };

        m_simplePipeline = CreatePipeline(device, surfaceExtent, renderPass, pipelineLayout, gridShaders[0], gridShaders[1], msaaSamples);
        SetResourceName(device, VK_OBJECT_TYPE_PIPELINE, m_simplePipeline, "LightingPass-Pipeline-Simple");

        vkDestroyShaderModule(device, gridShaders[0], nullptr);
        vkDestroyShaderModule(device, gridShaders[1], nullptr);
    }

    // shadow map lightning
    {
        VkShaderModule gridShaders[] = {
            CreateShaderModule(device, SPV_lightning_shadowmap_vert, sizeof(SPV_lightning_shadowmap_vert)),
            CreateShaderModule(device, SPV_lightning_shadowmap_frag, sizeof(SPV_lightning_shadowmap_frag)),
        };

        m_shadowMapPipeline = CreatePipeline(device, surfaceExtent, renderPass, pipelineLayout, gridShaders[0], gridShaders[1], msaaSamples);
        SetResourceName(device, VK_OBJECT_TYPE_PIPELINE, m_simplePipeline, "LightingPass-Pipeline-Shadow");

        vkDestroyShaderModule(device, gridShaders[0], nullptr);
        vkDestroyShaderModule(device, gridShaders[1], nullptr);
    }
}

void LightningPass::Destroy(const VkDevice device) {
    vkDestroyPipeline(device, m_simplePipeline, nullptr);
    vkDestroyPipeline(device, m_shadowMapPipeline, nullptr);
}
