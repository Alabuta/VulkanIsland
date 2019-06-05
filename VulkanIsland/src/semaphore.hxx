#pragma once

#include <iostream>
#include <optional>

#include "main.hxx"
#include "device/device.hxx"


std::optional<VkSemaphore> CreateSemaphore(VulkanDevice const &device) noexcept
{
    VkSemaphoreCreateInfo constexpr createInfo{
        VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        nullptr, 0
    };

    VkSemaphore handle;

    if (auto result = vkCreateSemaphore(device.handle(), &createInfo, nullptr, &handle); result == VK_SUCCESS)
        return handle;

    else std::cerr << "failed to create a semaphore: "s << result << '\n';

    return { };
}
