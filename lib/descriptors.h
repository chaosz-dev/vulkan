#pragma once

#include <vector>
#include <unordered_map>

#include <vulkan/vulkan_core.h>

class DescriptorSetMgmt;

class DescriptorMgmt {
public:
    DescriptorMgmt();

    void SetDescriptor(uint32_t binding, VkDescriptorType type, uint32_t count = 1);

    VkDescriptorSetLayout CreateLayout(const VkDevice device);
    VkDescriptorSetLayout Layout() const { return m_layout; }

    void CreatePool(const VkDevice device);
    void CreateDescriptorSets(const VkDevice device, uint32_t count);

    DescriptorSetMgmt &Set(uint32_t idx) { return m_sets[idx]; }

    void Destroy(const VkDevice device);

private:
    void AddDescType(VkDescriptorType type, uint32_t count);

    std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding>  m_bindings;

    std::unordered_map<VkDescriptorType, uint32_t>              m_descTypes;
    VkDescriptorSetLayout                                       m_layout = VK_NULL_HANDLE;
    VkDescriptorPool                                            m_pool = VK_NULL_HANDLE;
    std::vector<DescriptorSetMgmt>                              m_sets;
};

class DescriptorSetMgmt {
public:
    DescriptorSetMgmt(const VkDescriptorSet set)
        : m_set(set)
    {}

    VkDescriptorSet &Get() { return m_set; }

    void SetBuffer(uint32_t idx, VkBuffer buffer);
    void SetImage(uint32_t      idx,
                  VkImageView   view,
                  VkSampler     sampler,
                  VkImageLayout layout = VK_IMAGE_LAYOUT_GENERAL);

    void Update(const VkDevice device);

private:
    VkDescriptorSet                                         m_set;
    std::unordered_map<uint32_t, VkDescriptorBufferInfo>    m_bufferInfos;
    std::unordered_map<uint32_t, VkDescriptorImageInfo>     m_imageInfos;
};
