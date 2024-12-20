#include "shader_tooling.h"

VkShaderModule CreateShaderModule(
    const VkDevice  device,
    const uint32_t* SPIRVBinary,
    uint32_t    SPIRVBinarySize) {
    // SPIRVBinarySize is in bytes

    VkShaderModuleCreateInfo shaderModuleCreateInfo = {
        .sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .pNext    = nullptr,
        .flags    = 0,
        .codeSize = SPIRVBinarySize,
        .pCode    = SPIRVBinary,
    };

    VkShaderModule shaderModule = VK_NULL_HANDLE;
    vkCreateShaderModule(device, &shaderModuleCreateInfo, nullptr, &shaderModule);
    // TODO: result check
    return shaderModule;
}

