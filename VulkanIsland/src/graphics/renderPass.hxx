#pragma once

#include "main.hxx"
#include "renderer/device.hxx"
#include "renderer/swapchain.hxx"

[[nodiscard]] std::optional<VkRenderPass>
CreateRenderPass(VulkanDevice const &device, VulkanSwapchain const &swapchain) noexcept;
