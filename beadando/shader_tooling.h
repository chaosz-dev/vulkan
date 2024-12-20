#pragma once

#include <vulkan/vulkan_core.h>

VkShaderModule CreateShaderModule(
    const VkDevice  device,
    const uint32_t* SPIRVBinary,
    uint32_t        SPIRVBinarySize);
