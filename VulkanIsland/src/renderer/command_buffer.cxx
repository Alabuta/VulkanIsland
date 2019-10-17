#include "command_buffer.hxx"


VkCommandBuffer BeginSingleTimeCommand(vulkan::device const &device, VkCommandPool commandPool)
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
        throw std::runtime_error(fmt::format("failed to create allocate command buffers: {0:#x}\n"s, result));

    VkCommandBufferBeginInfo const beginInfo{
        VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        nullptr,
        VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
        nullptr
    };

    if (auto result = vkBeginCommandBuffer(commandBuffer, &beginInfo); result != VK_SUCCESS)
        throw std::runtime_error(fmt::format("failed to record command buffer: {0:#x}\n"s, result));

    return commandBuffer;
}

void EndSingleTimeCommand(VkCommandBuffer commandBuffer)
{
    if (auto result = vkEndCommandBuffer(commandBuffer); result != VK_SUCCESS)
        throw std::runtime_error(fmt::format("failed to end command buffer: {0:#x}\n"s, result));
}