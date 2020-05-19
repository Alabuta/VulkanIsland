#pragma once

#include <optional>
#include <iostream>
#include <memory>

#include <fmt/format.h>

#include "main.hxx"
#include "utility/exceptions.hxx"
#include "vulkan/device.hxx"


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


    DescriptorManager(vulkan::device &device) noexcept : device_{device} { }

    [[nodiscard]] std::optional<VulkanDescriptorPool> CreateDescriptorPool();

private:

    vulkan::device &device_;
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

std::optional<VkDescriptorPool> CreateDescriptorPool(vulkan::device const &device);

std::optional<VkDescriptorSetLayout> CreateDescriptorSetLayout(vulkan::device const &device);

template<mpl::container T>
std::optional<VkDescriptorSet>
CreateDescriptorSets(vulkan::device const &device, VkDescriptorPool descriptorPool, T &&descriptorSetLayouts)
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
        throw vulkan::exception(fmt::format("failed to allocate descriptor set(s): {0:#x}"s, result));

    else descriptorSet.emplace(handle);

    return descriptorSet;
}
