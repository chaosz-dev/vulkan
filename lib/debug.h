#pragma once
#include <string>

template<typename T>
void SetResourceName(const VkDevice device, VkObjectType objectType, const T resource, const std::string& name) {
    VkDebugUtilsObjectNameInfoEXT info = {
        .sType          = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
        .pNext          = nullptr,
        .objectType     = objectType,
        .objectHandle   = reinterpret_cast<uint64_t>(resource),
        .pObjectName    = name.c_str(),
    };

#define VK_LOAD_INSTANCE_PFN(instance, name) reinterpret_cast<PFN_##name>(vkGetInstanceProcAddr(instance, #name))

    static PFN_vkSetDebugUtilsObjectNameEXT vkSetDebugUtilsObjectNameEXT
        = reinterpret_cast<PFN_vkSetDebugUtilsObjectNameEXT>(vkGetDeviceProcAddr(device, "vkSetDebugUtilsObjectNameEXT"));

    vkSetDebugUtilsObjectNameEXT(device, &info);
}
