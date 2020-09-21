#pragma once

#include <optional>
#include <iostream>
#include <memory>
#include <span>

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

    [[nodiscard]] std::optional<VulkanDescriptorPool> create_descriptor_pool();

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

std::optional<VkDescriptorPool> create_descriptor_pool(vulkan::device const &device);

std::optional<VkDescriptorSetLayout> CreateDescriptorSetLayout(vulkan::device const &device);

std::optional<VkDescriptorSet>
create_descriptor_sets(vulkan::device const &device, VkDescriptorPool descriptorPool, std::span<VkDescriptorSetLayout const> const descriptorSetLayouts);
