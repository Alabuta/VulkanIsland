#pragma once

#include <optional>
#include <memory>

#include "main.hxx"
#include "device.hxx"

class DescriptorManager;

class VulkanDescriptorPool final {
public:

    VkDescriptorPool handle() const noexcept { return handle_; }

    VulkanDescriptorPool(VkDescriptorPool handle) noexcept : handle_{handle} { }

private:
    VkDescriptorPool handle_;

    VulkanDescriptorPool() = delete;
    VulkanDescriptorPool(VulkanDescriptorPool const &) = delete;
    VulkanDescriptorPool(VulkanDescriptorPool &&) = delete;

    friend DescriptorManager;
};

class DescriptorManager final {
public:


    DescriptorManager(VulkanDevice &device) noexcept : device_{device} { }

    [[nodiscard]] std::optional<VulkanDescriptorPool> CreateDescriptorPool();

private:

    VulkanDevice &device_;
};

#if 0
class VulkanDescriptorSet final {
public:

    VulkanDescriptorSet() = default;
//    VulkanDescriptorSet(VkImageView handle, VkImageViewType type) noexcept : handle_{handle}, type_{type} { }

    VkDescriptorSet handle() const noexcept { return handle_; }

private:
    VkDescriptorSet handle_;
};
#endif

std::optional<VkDescriptorPool> CreateDescriptorPool(VulkanDevice const &device);

std::optional<VkDescriptorSetLayout> CreateDescriptorSetLayout(VulkanDevice const &device);

template<class T, typename std::enable_if_t<is_container_v<std::decay_t<T>>>...>
std::optional<VkDescriptorSet> CreateDescriptorSet(VulkanDevice const &device, VkDescriptorPool descriptorPool, T &&descriptorSetLayouts)
{
    std::optional<VkDescriptorSet> descriptorSet;

    VkDescriptorSetAllocateInfo const allocateInfo{
        VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        nullptr,
        descriptorPool,
        static_cast<std::uint32_t>(std::size(descriptorSetLayouts)), std::data(descriptorSetLayouts)
    };

    VkDescriptorSet handle;

    if (auto result = vkAllocateDescriptorSets(device.handle(), &allocateInfo, &handle); result != VK_SUCCESS)
        std::cerr << "failed to allocate descriptor set(s): "s << result << '\n';

    else descriptorSet.emplace(handle);

    return descriptorSet;
}
