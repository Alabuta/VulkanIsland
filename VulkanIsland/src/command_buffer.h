#pragma once

#include "main.h"
#include "device.h"

template<class Q, typename std::enable_if_t<std::is_base_of_v<VulkanQueue<Q>, std::decay_t<Q>>>...>
[[nodiscard]] VkCommandBuffer BeginSingleTimeCommand(VulkanDevice const &device, [[maybe_unused]] Q &queue, VkCommandPool commandPool)
{
    VkCommandBuffer commandBuffer;

    VkCommandBufferAllocateInfo const allocateInfo{
        VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        nullptr,
        commandPool,
        VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        1
    };

    if (auto result = vkAllocateCommandBuffers(device.handle(), &allocateInfo, &commandBuffer); result != VK_SUCCESS)
        throw std::runtime_error("failed to create allocate command buffers: "s + std::to_string(result));

    VkCommandBufferBeginInfo const beginInfo{
        VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        nullptr,
        VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
        nullptr
    };

    if (auto result = vkBeginCommandBuffer(commandBuffer, &beginInfo); result != VK_SUCCESS)
        throw std::runtime_error("failed to record command buffer: "s + std::to_string(result));

    return commandBuffer;
}

template<class Q, typename std::enable_if_t<std::is_base_of_v<VulkanQueue<Q>, std::decay_t<Q>>>...>
void EndSingleTimeCommand(VulkanDevice const &device, Q &queue, VkCommandBuffer commandBuffer, VkCommandPool commandPool)
{
    if (auto result = vkEndCommandBuffer(commandBuffer); result != VK_SUCCESS)
        throw std::runtime_error("failed to end command buffer: "s + std::to_string(result));

    VkSubmitInfo const submitInfo{
        VK_STRUCTURE_TYPE_SUBMIT_INFO,
        nullptr,
        0, nullptr,
        nullptr,
        1, &commandBuffer,
        0, nullptr,
    };

    if (auto result = vkQueueSubmit(queue.handle(), 1, &submitInfo, VK_NULL_HANDLE); result != VK_SUCCESS)
        throw std::runtime_error("failed to submit command buffer: "s + std::to_string(result));

    vkQueueWaitIdle(queue.handle());

    vkFreeCommandBuffers(device.handle(), commandPool, 1, &commandBuffer);
}
