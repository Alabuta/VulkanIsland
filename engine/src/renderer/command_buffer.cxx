#include "command_buffer.hxx"


VkCommandBuffer begin_single_time_command(vulkan::device const &device, VkCommandPool command_pool)
{
    VkCommandBuffer command_buffer;

    VkCommandBufferAllocateInfo const allocate_info{
        VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        nullptr,
        command_pool,
        VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        1
    };

    if (auto result = vkAllocateCommandBuffers(device.handle(), &allocate_info, &command_buffer); result != VK_SUCCESS)
        throw vulkan::exception(fmt::format("failed to create allocate command buffers: {0:#x}", result));

    VkCommandBufferBeginInfo const begin_info{
        VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        nullptr,
        VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
        nullptr
    };

    if (auto result = vkBeginCommandBuffer(command_buffer, &begin_info); result != VK_SUCCESS)
        throw vulkan::exception(fmt::format("failed to record command buffer: {0:#x}", result));

    return command_buffer;
}

void end_single_time_command(VkCommandBuffer command_buffer)
{
    if (auto result = vkEndCommandBuffer(command_buffer); result != VK_SUCCESS)
        throw vulkan::exception(fmt::format("failed to end command buffer: {0:#x}", result));
}


void copy_buffer_to_image(vulkan::device const &device, graphics::transfer_queue const &queue,
                          VkBuffer src, VkImage dst, renderer::extent extent, VkCommandPool command_pool)
{
    auto command_buffer = begin_single_time_command(device, command_pool);

    auto [width, height] = extent;

    VkBufferImageCopy const copy_region{
        0,
        0, 0,
        { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 },
        { 0, 0, 0 },
        { width, height, 1 }
    };

    vkCmdCopyBufferToImage(command_buffer, src, dst, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy_region);

    end_single_time_command(command_buffer);

    submit_and_free_single_time_command_buffer(device, queue, command_pool, command_buffer);
}

void copy_buffer_to_buffer(vulkan::device const &device, graphics::transfer_queue const &queue,
                           VkBuffer src, VkBuffer dst, std::span<VkBufferCopy const> const copy_region, VkCommandPool command_pool)
{
    auto command_buffer = begin_single_time_command(device, command_pool);

    vkCmdCopyBuffer(command_buffer, src, dst, static_cast<std::uint32_t>(std::size(copy_region)), std::data(copy_region));

    end_single_time_command(command_buffer);

    submit_and_free_single_time_command_buffer(device, queue, command_pool, command_buffer);
}

