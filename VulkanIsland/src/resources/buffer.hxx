#pragma once

#include "main.hxx"
#include "memory.hxx"


class VulkanBuffer final {
public:

    VulkanBuffer(std::shared_ptr<DeviceMemory> memory, VkBuffer handle) : memory_{memory}, handle_{handle} { }

    std::shared_ptr<DeviceMemory> memory() const noexcept { return memory_; }
    std::shared_ptr<DeviceMemory> &memory() noexcept { return memory_; }

    VkBuffer handle() const noexcept { return handle_; }

private:
    std::shared_ptr<DeviceMemory> memory_;
    VkBuffer handle_;

    VulkanBuffer() = delete;
    VulkanBuffer(VulkanBuffer const &) = delete;
    VulkanBuffer(VulkanBuffer &&) = delete;
};
