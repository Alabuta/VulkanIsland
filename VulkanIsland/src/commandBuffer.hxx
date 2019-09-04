#pragma once

#include "main.hxx"
#include "device/device.hxx"
#include "resources/image.hxx"


template<class Q, typename std::enable_if_t<std::is_base_of_v<VulkanQueue<Q>, std::decay_t<Q>>>* = nullptr>
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

template<class Q, typename std::enable_if_t<std::is_base_of_v<VulkanQueue<Q>, std::decay_t<Q>>>* = nullptr>
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


template<class Q, class R, typename std::enable_if_t<std::is_base_of_v<VulkanQueue<Q>, std::decay_t<Q>> && is_container_v<std::decay_t<R>>>* = nullptr>
void CopyBufferToBuffer(VulkanDevice const &device, Q &queue, VkBuffer srcBuffer, VkBuffer dstBuffer, R &&copyRegion, VkCommandPool commandPool)
{
    static_assert(std::is_same_v<typename std::decay_t<R>::value_type, VkBufferCopy>, "'copyRegion' argument does not contain 'VkBufferCopy' elements");

    auto commandBuffer = BeginSingleTimeCommand(device, queue, commandPool);

    vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, static_cast<std::uint32_t>(std::size(copyRegion)), std::data(copyRegion));

    EndSingleTimeCommand(device, queue, commandBuffer, commandPool);
}

template<class Q, typename std::enable_if_t<std::is_base_of_v<VulkanQueue<Q>, std::decay_t<Q>>>* = nullptr>
void CopyBufferToImage(VulkanDevice const &device, Q &queue, VkBuffer srcBuffer, VkImage dstImage, std::uint16_t width, std::uint16_t height, VkCommandPool commandPool)
{
    auto commandBuffer = BeginSingleTimeCommand(device, queue, commandPool);

    VkBufferImageCopy const copyRegion{
        0,
        0, 0,
        { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 },
        { 0, 0, 0 },
        { width, height, 1 }
    };

    vkCmdCopyBufferToImage(commandBuffer, srcBuffer, dstImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);

    EndSingleTimeCommand(device, queue, commandBuffer, commandPool);
}


template<class Q, typename std::enable_if_t<std::is_base_of_v<VulkanQueue<Q>, std::decay_t<Q>>>* = nullptr>
void GenerateMipMaps(VulkanDevice const &device, Q &queue, VulkanImage const &image, VkCommandPool commandPool) noexcept
{
    auto commandBuffer = BeginSingleTimeCommand(device, queue, commandPool);

    auto width = image.width();
    auto height = image.height();

    VkImageMemoryBarrier barrier{
        VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        nullptr,
        0, 0,
        VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_UNDEFINED,
        VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
        image.handle(),
        { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 }
    };

    for (auto i = 1u; i < image.mipLevels(); ++i) {
        barrier.subresourceRange.baseMipLevel = i - 1;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

        vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

        VkImageBlit const imageBlit{
            { VK_IMAGE_ASPECT_COLOR_BIT, i - 1, 0, 1 },
            {{ 0, 0, 0 }, {width, height, 1 }},
            { VK_IMAGE_ASPECT_COLOR_BIT, i, 0, 1 },
            {{ 0, 0, 0 }, {width / 2, height / 2, 1 }}
        };

        vkCmdBlitImage(commandBuffer, image.handle(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, image.handle(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imageBlit, VK_FILTER_LINEAR);

        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

        if (width > 1) width /= 2;
        if (height > 1) height /= 2;
    }

    barrier.subresourceRange.baseMipLevel = image.mipLevels() - 1;
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

    EndSingleTimeCommand(device, queue, commandBuffer, commandPool);
}


template<class Q, typename std::enable_if_t<std::is_base_of_v<VulkanQueue<Q>, std::decay_t<Q>>>* = nullptr>
bool TransitionImageLayout(VulkanDevice const &device, Q &queue, VulkanImage const &image,
                           VkImageLayout srcLayout, VkImageLayout dstLayout, VkCommandPool commandPool) noexcept
{
    VkImageMemoryBarrier barrier{
        VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        nullptr,
        0, 0,
        srcLayout, dstLayout,
        VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
        image.handle(),
        { VK_IMAGE_ASPECT_COLOR_BIT, 0, image.mipLevels(), 0, 1 }
    };

    if (dstLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

        if (image.format() == graphics::FORMAT::D32_SFLOAT_S8_UINT || image.format() == graphics::FORMAT::D24_UNORM_S8_UINT)
            barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
    }

    VkPipelineStageFlags srcStageFlags, dstStageFlags;

    if (srcLayout == VK_IMAGE_LAYOUT_UNDEFINED && dstLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        srcStageFlags = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        dstStageFlags = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }

    else if (srcLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && dstLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        srcStageFlags = VK_PIPELINE_STAGE_TRANSFER_BIT;
        dstStageFlags = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }

    else if (srcLayout == VK_IMAGE_LAYOUT_UNDEFINED && dstLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        srcStageFlags = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        dstStageFlags = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    }

    else if (srcLayout == VK_IMAGE_LAYOUT_UNDEFINED && dstLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        srcStageFlags = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        dstStageFlags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    }

    else {
        std::cerr << "unsupported layout transition\n"s;
        return false;
    }

    auto commandBuffer = BeginSingleTimeCommand(device, queue, commandPool);

    vkCmdPipelineBarrier(commandBuffer, srcStageFlags, dstStageFlags, 0, 0, nullptr, 0, nullptr, 1, &barrier);

    EndSingleTimeCommand(device, queue, commandBuffer, commandPool);

    return true;
}


template<class Q, typename std::enable_if_t<std::is_base_of_v<VulkanQueue<Q>, std::decay_t<Q>>>* = nullptr>
std::optional<VkCommandPool> CreateCommandPool(VkDevice device, Q &queue, VkCommandPoolCreateFlags flags)
{
    VkCommandPoolCreateInfo const createInfo{
        VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        nullptr,
        flags,
        queue.family()
    };

    VkCommandPool handle;

    if (auto result = vkCreateCommandPool(device, &createInfo, nullptr, &handle); result != VK_SUCCESS)
        std::cerr << "failed to create a command buffer: "s << result << std::endl;

    else return handle;

    return { };
}

