#pragma once

#include "main.hxx"
#include "vulkan/device.hxx"
#include "renderer/swapchain.hxx"

[[nodiscard]] std::optional<VkRenderPass>
CreateRenderPass(vulkan::device const &device, VulkanSwapchain const &swapchain) noexcept;
