#include "shadow_map.h"

#include <cassert>

#include "shader_tooling.h"

namespace {
#include "shadow_map.vert_include.h"
#include "shadow_map.frag_include.h"
}


bool ShadowMap::Build(const VkPhysicalDevice phyDevice, const VkDevice device, uint32_t size) {
    m_extent = { size, size };

    m_shadowDepth = Texture::Create2D(phyDevice, device, m_depthFormat, m_extent,
                                      VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
    assert(m_shadowDepth.IsValid());
    BuildRenderpass(device);
    BuildFBO(device);

    return true;
}

bool ShadowMap::BuildPipeline(const VkDevice device, const VkPipelineLayout pipelineLayout) {
    VkShaderModule shaderVertex     = CreateShaderModule(device, SPV_shadow_map_vert, sizeof(SPV_shadow_map_vert));
    VkShaderModule shaderFragment   = CreateShaderModule(device, SPV_shadow_map_frag, sizeof(SPV_shadow_map_frag));

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

    // Vertex Infor information must match across all objects!
    // IMPORTANT!
    VkVertexInputBindingDescription vertexBinding = {
        // binding position in Vulkan API, maches the binding in VkVertexInputAttributeDescription
        .binding   = 0,
        .stride    = sizeof(float) * (3 + 2 + 3), // step by "vec3 + vec2 + vec3" elements in the attached buffer
        .inputRate = VK_VERTEX_INPUT_RATE_VERTEX, // step "stride" bytes each for vertex in the buffer
    };

    // IMPORTANT!
    VkVertexInputAttributeDescription vertexAttributes[3] = {
        {   // position
            .location = 0,                          // layout location=0 in shader
            .binding  = 0,                          // binding position in Vulkan API
            .format   = VK_FORMAT_R32G32B32_SFLOAT, // use "vec3" values from the buffer
            .offset   = 0,                          // use buffer from the 0 byte
        },
        {   // uv
            .location = 1,                          // layout location=1 in shader
            .binding  = 0,                          // binding position in Vulkan API
            .format   = VK_FORMAT_R32G32_SFLOAT,    // use "vec2" values from the buffer
            .offset   = sizeof(float) * 3,          // use buffer from the 0 byte
        },
        {   // normals
            .location = 2,                          // layout location=2 in shader
            .binding  = 0,                          // binding position in Vulkan API
            .format   = VK_FORMAT_R32G32B32_SFLOAT, // use "vec3" values from the buffer
            .offset   = sizeof(float) * (3 + 2),    // use buffer from the 0 byte
        },
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
        .width      = float(m_extent.width),
        .height     = float(m_extent.height),
        .minDepth   = 0.0f,
        .maxDepth   = 1.0f,
    };

    VkRect2D scissor {
        .offset = { 0, 0 },
        .extent = m_extent,
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
        .sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .pNext                   = nullptr,
        .flags                   = 0,
        .depthClampEnable        = VK_FALSE,
        .rasterizerDiscardEnable = VK_FALSE,
        .polygonMode             = VK_POLYGON_MODE_FILL,
        .cullMode                = VK_CULL_MODE_NONE, //VK_CULL_MODE_FRONT_BIT,
        .frontFace               = VK_FRONT_FACE_CLOCKWISE,
        .depthBiasEnable         = VK_TRUE,  // TASK: Test with true
        .depthBiasConstantFactor = 1.25f, // Disabled
        .depthBiasClamp          = 0.0f, // Disabled
        .depthBiasSlopeFactor    = 1.75f, // Disabled
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
    VkStencilOpState emptyStencilOp = { }; // Removed memeber init for space

    VkPipelineDepthStencilStateCreateInfo depthStencilInfo = {
        .sType                 = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .pNext                 = nullptr,
        .flags                 = 0,
        .depthTestEnable       = VK_TRUE, // Depth test is a must!
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
        .blendEnable         = VK_FALSE,
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
        .logicOpEnable   = VK_FALSE,
        .logicOp         = VK_LOGIC_OP_CLEAR,
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
        .renderPass          = m_renderPass,
        .subpass             = 0,
        .basePipelineHandle  = VK_NULL_HANDLE,
        .basePipelineIndex   = 0,
    };

    VkResult result = vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &m_pipeline);

    vkDestroyShaderModule(device, shaderVertex, nullptr);
    vkDestroyShaderModule(device, shaderFragment, nullptr);

    return result == VK_SUCCESS;
}


bool ShadowMap::BuildRenderpass(const VkDevice device) {

    const VkAttachmentDescription attachments[] = {
        { // 0. depth
            .flags          = 0,
            .format         = m_depthFormat,
            .samples        = VK_SAMPLE_COUNT_1_BIT,
            .loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp        = VK_ATTACHMENT_STORE_OP_STORE,
            .stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED,
            .finalLayout    = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL,
        },
    };

    VkAttachmentReference depthAttachmentRef = {
        .attachment = 0,
        .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
    };

    const VkSubpassDescription subpass = {
        .flags                       = 0,
        .pipelineBindPoint          = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .inputAttachmentCount       = 0,
        .pInputAttachments          = NULL,
        .colorAttachmentCount       = 0,
        .pColorAttachments          = NULL,
        .pResolveAttachments        = NULL,
        .pDepthStencilAttachment    = &depthAttachmentRef,
        .preserveAttachmentCount    = 0,
        .pPreserveAttachments       = NULL,
    };

    VkRenderPassCreateInfo createInfo = {
        .sType              = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .pNext              = nullptr,
        .flags              = 0,
        .attachmentCount    = std::size(attachments),
        .pAttachments       = attachments,
        .subpassCount       = 1,
        .pSubpasses         = &subpass,
        .dependencyCount    = 0,
        .pDependencies      = NULL,
    };

    return vkCreateRenderPass(device, &createInfo, nullptr, &m_renderPass) == VK_SUCCESS;
}

bool ShadowMap::BuildFBO(const VkDevice device) {
    VkImageView attachments[] = {
        m_shadowDepth.view(),
    };

    VkFramebufferCreateInfo createInfo = {
        .sType              = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .pNext              = NULL,
        .flags              = 0,
        .renderPass         = m_renderPass,
        .attachmentCount    = 1,
        .pAttachments       = attachments, // updated below
        .width              = m_extent.width,
        .height             = m_extent.height,
        .layers             = 1,
    };

    VkResult result = vkCreateFramebuffer(device, &createInfo, nullptr, &m_framebuffer);
    return result == VK_SUCCESS;
}

void ShadowMap::Destroy(const VkDevice device) {
    m_shadowDepth.Destroy(device);
    vkDestroyPipeline(device, m_pipeline, nullptr);
    vkDestroyFramebuffer(device, m_framebuffer, nullptr);
    vkDestroyRenderPass(device, m_renderPass, nullptr);
}

void ShadowMap::BeginPass(const VkCommandBuffer cmdBuffer) {
    VkClearValue clears[1];
    clears[0].depthStencil = {1.0f, 0};

    // Shadow
    VkRenderPassBeginInfo shadowRenderPassInfo = {
        .sType          = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .pNext          = nullptr,
        .renderPass     = m_renderPass,
        .framebuffer    = m_framebuffer,
        .renderArea     = {
            .offset = { 0, 0 },
            .extent = m_extent,
        },
        .clearValueCount = 1,
        .pClearValues = clears,
    };

    vkCmdBeginRenderPass(cmdBuffer, &shadowRenderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdSetViewport(cmdBuffer, 0, 1, &Viewport());
    vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, Pipeline());
}
