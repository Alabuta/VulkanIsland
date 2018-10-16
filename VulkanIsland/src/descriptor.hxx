#pragma once

#include <optional>
#include <memory>

#include "main.hxx"
#include "device.hxx"

class DescriptorsManager;

class VulkanDescriptorPool final {
public:

    VkDescriptorPool handle() const noexcept { return handle_; }

    VulkanDescriptorPool(VkDescriptorPool handle) noexcept : handle_{handle} { }

private:
    VkDescriptorPool handle_;

    VulkanDescriptorPool() = delete;
    VulkanDescriptorPool(VulkanDescriptorPool const &) = delete;
    VulkanDescriptorPool(VulkanDescriptorPool &&) = delete;

    friend DescriptorsManager;
};

class DescriptorsManager final {
public:


    DescriptorsManager(VulkanDevice &device) noexcept : device_{device} { }

    [[nodiscard]] std::shared_ptr<VulkanDescriptorPool> CreateDescriptorPool();

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
