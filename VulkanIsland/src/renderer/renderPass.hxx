#pragma once

#include "main.hxx"
#include "device/device.hxx"
#include "swapchain.hxx"

[[nodiscard]] std::optional<VkRenderPass>
CreateRenderPass(VulkanDevice const &device, VulkanSwapchain const &swapchain) noexcept;
