#include "descriptors.h"

#include <cassert>

DescriptorMgmt::DescriptorMgmt() {
}

void DescriptorMgmt::SetDescriptor(
    uint32_t            bindingIdx,
    VkDescriptorType    type,
    uint32_t            count) {

    VkDescriptorSetLayoutBinding binding = {
        .binding            = bindingIdx,
        .descriptorType     = type,
        .descriptorCount    = count,
        .stageFlags         = VK_SHADER_STAGE_ALL,
        .pImmutableSamplers = nullptr,
    };

    m_bindings[bindingIdx] = binding;

    AddDescType(type, count);
}

void DescriptorMgmt::AddDescType(
    VkDescriptorType    type,
    uint32_t            count) {
    m_descTypes[type] += count;
}

void DescriptorMgmt::CreatePool(const VkDevice device) {
    std::vector<VkDescriptorPoolSize> poolSizes;
    poolSizes.reserve(m_descTypes.size());

    for (const std::pair<const VkDescriptorType, uint32_t> &entry : m_descTypes) {
        poolSizes.push_back({entry.first, entry.second});
    }

    VkDescriptorPoolCreateInfo createInfo = {
        .sType          = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .pNext          = nullptr,
        .flags          = 0,
        .maxSets        = 4,
        .poolSizeCount  = (uint32_t)poolSizes.size(),
        .pPoolSizes     = poolSizes.data(),
    };

    VkResult result = vkCreateDescriptorPool(device, &createInfo, nullptr, &m_pool);
    if (result != VK_SUCCESS) {
        // TODO: ....
    }
}

VkDescriptorSetLayout DescriptorMgmt::CreateLayout(const VkDevice device) {
    std::vector<VkDescriptorSetLayoutBinding> bindings;
    bindings.reserve(m_bindings.size());

    for (const std::pair<uint32_t, VkDescriptorSetLayoutBinding> entry : m_bindings) {
        bindings.push_back(entry.second);
    }

    VkDescriptorSetLayoutCreateInfo createInfo = {
        .sType          = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .pNext          = nullptr,
        .flags          = 0,
        .bindingCount   = (uint32_t)bindings.size(),
        .pBindings      = bindings.data(),
    };

    VkResult result = vkCreateDescriptorSetLayout(device, &createInfo, nullptr, &m_layout);
    if (result != VK_SUCCESS) {
        // TODO: ....
    }

    return m_layout;
}

void DescriptorMgmt::CreateDescriptorSets(const VkDevice device, uint32_t count) {
    assert(m_pool != VK_NULL_HANDLE);
    assert(m_layout != VK_NULL_HANDLE);

    std::vector<VkDescriptorSetLayout> layouts(count, m_layout);

    VkDescriptorSetAllocateInfo createInfo = {
        .sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .pNext              = nullptr,
        .descriptorPool     = m_pool,
        .descriptorSetCount = count,
        .pSetLayouts        = layouts.data(),
    };

    std::vector<VkDescriptorSet> sets(count, VK_NULL_HANDLE);

    VkResult result = vkAllocateDescriptorSets(device, &createInfo, sets.data());
    if (result != VK_SUCCESS) {
        // TODO: ....
    }

    for (const VkDescriptorSet &set : sets) {
        m_sets.push_back(DescriptorSetMgmt(set));
    }
}

void DescriptorMgmt::Destroy(const VkDevice device) {
    vkDestroyDescriptorPool(device, m_pool, nullptr);

    vkDestroyDescriptorSetLayout(device, m_layout, nullptr);
}


void DescriptorSetMgmt::SetBuffer(uint32_t idx, VkBuffer buffer) {
    m_bufferInfos[idx] = { buffer, 0, VK_WHOLE_SIZE };
}

void DescriptorSetMgmt::SetImage(
    uint32_t      idx,
    VkImageView   view,
    VkSampler     sampler,
    VkImageLayout layout) {
    m_imageInfos[idx] = { sampler, view, layout };
}

void DescriptorSetMgmt::Update(const VkDevice device) {
    VkWriteDescriptorSet baseInfo = {
        .sType              = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .pNext              = nullptr,
        .dstSet             = m_set,
        .dstBinding         = 0,    // set later
        .dstArrayElement    = 0,
        .descriptorCount    = 1,    // set later
        .descriptorType     = (VkDescriptorType)0,    // set later
        .pImageInfo         = nullptr,
        .pBufferInfo        = nullptr,
        .pTexelBufferView   = nullptr,
    };

    const uint32_t infoCount = (uint32_t)(m_bufferInfos.size() + m_imageInfos.size());
    std::vector<VkWriteDescriptorSet> writeInfos(infoCount, baseInfo);

    for (const std::pair<const uint32_t, VkDescriptorBufferInfo> &entry : m_bufferInfos) {
        const uint32_t                  idx = entry.first;
        const VkDescriptorBufferInfo&   info = entry.second;

        VkWriteDescriptorSet &writeInfo = writeInfos[idx];

        writeInfo.dstBinding     = idx;
        writeInfo.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        writeInfo.pBufferInfo     = &info;
    }

    for (const std::pair<const uint32_t, VkDescriptorImageInfo> &entry : m_imageInfos) {
        const uint32_t                  idx = entry.first;
        const VkDescriptorImageInfo&    info = entry.second;

        VkWriteDescriptorSet &writeInfo = writeInfos[idx];

        writeInfo.dstBinding     = idx;
        writeInfo.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        writeInfo.pImageInfo     = &info;
    }

    vkUpdateDescriptorSets(device, infoCount, writeInfos.data(), 0, nullptr);
}
