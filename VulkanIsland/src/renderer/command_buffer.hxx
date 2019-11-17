#pragma once

#include <iostream>
#include <string>
using namespace std::string_literals;

#include <fmt/format.h>

#include "main.hxx"
#include "vulkan/device.hxx"
#include "resources/image.hxx"
#include "graphics/graphics.hxx"
#include "graphics/graphics_api.hxx"


[[nodiscard]] VkCommandBuffer BeginSingleTimeCommand(vulkan::device const &device, VkCommandPool commandPool);

void EndSingleTimeCommand(VkCommandBuffer commandBuffer);

template<class Q, typename std::enable_if_t<std::is_base_of_v<graphics::queue, std::remove_cvref_t<Q>>>* = nullptr>
void SubmitAndFreeSingleTimeCommandBuffers(vulkan::device const &device, Q &queue, VkCommandPool commandPool, VkCommandBuffer &commandBuffer)
{
    VkSubmitInfo const submitInfo{
        VK_STRUCTURE_TYPE_SUBMIT_INFO,
        nullptr,
        0, nullptr,
        nullptr,
        1, &commandBuffer,
        0, nullptr,
    };

    if (auto result = vkQueueSubmit(queue.handle(), 1, &submitInfo, VK_NULL_HANDLE); result != VK_SUCCESS)
        throw std::runtime_error(fmt::format("failed to submit command buffer: {0:#x}\n"s, result));

    vkQueueWaitIdle(queue.handle());

    vkFreeCommandBuffers(device.handle(), commandPool, 1, &commandBuffer);
}


template<class Q, class R, typename std::enable_if_t<std::is_base_of_v<graphics::queue, std::remove_cvref_t<Q>> && mpl::container<std::remove_cvref_t<R>>>* = nullptr>
void CopyBufferToBuffer(vulkan::device const &device, Q &queue, VkBuffer srcBuffer, VkBuffer dstBuffer, R &&copyRegion, VkCommandPool commandPool)
{
    static_assert(std::is_same_v<typename std::remove_cvref_t<R>::value_type, VkBufferCopy>, "'copyRegion' argument does not contain 'VkBufferCopy' elements");

    auto commandBuffer = BeginSingleTimeCommand(device, commandPool);

    vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, static_cast<std::uint32_t>(std::size(copyRegion)), std::data(copyRegion));

    EndSingleTimeCommand(commandBuffer);

    SubmitAndFreeSingleTimeCommandBuffers(device, queue, commandPool, commandBuffer);
}

template<class Q, typename std::enable_if_t<std::is_base_of_v<graphics::queue, std::remove_cvref_t<Q>>>* = nullptr>
void CopyBufferToImage(vulkan::device const &device, Q &queue, VkBuffer srcBuffer, VkImage dstImage, std::uint16_t width, std::uint16_t height, VkCommandPool commandPool)
{
    auto commandBuffer = BeginSingleTimeCommand(device, commandPool);

    VkBufferImageCopy const copyRegion{
        0,
        0, 0,
        { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 },
        { 0, 0, 0 },
        { width, height, 1 }
    };

    vkCmdCopyBufferToImage(commandBuffer, srcBuffer, dstImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);

    EndSingleTimeCommand(commandBuffer);

    SubmitAndFreeSingleTimeCommandBuffers(device, queue, commandPool, commandBuffer);
}


template<class Q, typename std::enable_if_t<std::is_base_of_v<graphics::queue, std::remove_cvref_t<Q>>>* = nullptr>
void GenerateMipMaps(vulkan::device const &device, Q &queue, resource::image const &image, VkCommandPool commandPool) noexcept
{
    auto commandBuffer = BeginSingleTimeCommand(device, commandPool);

    auto &&extent = image.extent();
    auto [width, height] = extent;

    VkImageMemoryBarrier barrier{
        VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        nullptr,
        0, 0,
        VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_UNDEFINED,
        VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
        image.handle(),
        { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 }
    };

    for (auto i = 1u; i < image.mip_levels(); ++i) {
        barrier.subresourceRange.baseMipLevel = i - 1;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

        vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

        VkImageBlit const imageBlit{
            { VK_IMAGE_ASPECT_COLOR_BIT, i - 1, 0, 1 },
            {{ 0, 0, 0 }, { static_cast<std::int32_t>(width), static_cast<std::int32_t>(height), 1 }},
            { VK_IMAGE_ASPECT_COLOR_BIT, i, 0, 1 },
            {{ 0, 0, 0 }, { static_cast<std::int32_t>(width / 2), static_cast<std::int32_t>(height / 2), 1 }}
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

    barrier.subresourceRange.baseMipLevel = image.mip_levels() - 1;
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

    EndSingleTimeCommand(commandBuffer);

    SubmitAndFreeSingleTimeCommandBuffers(device, queue, commandPool, commandBuffer);
}


template<class Q, typename std::enable_if_t<std::is_base_of_v<graphics::queue, std::remove_cvref_t<Q>>>* = nullptr>
bool TransitionImageLayout(vulkan::device const &device, Q &queue, resource::image const &image,
                           graphics::IMAGE_LAYOUT srcLayout, graphics::IMAGE_LAYOUT dstLayout, VkCommandPool commandPool) noexcept
{
    VkImageMemoryBarrier barrier{
        VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        nullptr,
        0, 0,
        convert_to::vulkan(srcLayout), convert_to::vulkan(dstLayout),
        VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
        image.handle(),
        { VK_IMAGE_ASPECT_COLOR_BIT, 0, image.mip_levels(), 0, 1 }
    };

    if (dstLayout == graphics::IMAGE_LAYOUT::DEPTH_STENCIL_ATTACHMENT) {
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

        if (image.format() == graphics::FORMAT::D32_SFLOAT_S8_UINT || image.format() == graphics::FORMAT::D24_UNORM_S8_UINT)
            barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
    }

    graphics::PIPELINE_STAGE srcStageFlags, dstStageFlags;

    if (srcLayout == graphics::IMAGE_LAYOUT::UNDEFINED && dstLayout == graphics::IMAGE_LAYOUT::TRANSFER_DESTINATION) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        srcStageFlags = graphics::PIPELINE_STAGE::TOP_OF_PIPE;
        dstStageFlags = graphics::PIPELINE_STAGE::TRANSFER;
    }

    else if (srcLayout == graphics::IMAGE_LAYOUT::TRANSFER_DESTINATION && dstLayout == graphics::IMAGE_LAYOUT::SHADER_READ_ONLY) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        srcStageFlags = graphics::PIPELINE_STAGE::TRANSFER;
        dstStageFlags = graphics::PIPELINE_STAGE::FRAGMENT_SHADER;
    }

    else if (srcLayout == graphics::IMAGE_LAYOUT::UNDEFINED && dstLayout == graphics::IMAGE_LAYOUT::DEPTH_STENCIL_ATTACHMENT) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        srcStageFlags = graphics::PIPELINE_STAGE::TOP_OF_PIPE;
        dstStageFlags = graphics::PIPELINE_STAGE::EARLY_FRAGMENT_TESTS;
    }

    else if (srcLayout == graphics::IMAGE_LAYOUT::UNDEFINED && dstLayout == graphics::IMAGE_LAYOUT::COLOR_ATTACHMENT) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        srcStageFlags = graphics::PIPELINE_STAGE::TOP_OF_PIPE;
        dstStageFlags = graphics::PIPELINE_STAGE::COLOR_ATTACHMENT_OUTPUT;
    }

    else {
        std::cerr << "unsupported layout transition\n"s;
        return false;
    }

    auto commandBuffer = BeginSingleTimeCommand(device, commandPool);

    vkCmdPipelineBarrier(commandBuffer, convert_to::vulkan(srcStageFlags), convert_to::vulkan(dstStageFlags), 0, 0, nullptr, 0, nullptr, 1, &barrier);

    EndSingleTimeCommand(commandBuffer);

    SubmitAndFreeSingleTimeCommandBuffers(device, queue, commandPool, commandBuffer);

    return true;
}


template<class Q, typename std::enable_if_t<std::is_base_of_v<graphics::queue, std::remove_cvref_t<Q>>>* = nullptr>
std::optional<VkCommandPool> CreateCommandPool(vulkan::device const &device, Q &queue, VkCommandPoolCreateFlags flags)
{
    VkCommandPoolCreateInfo const createInfo{
        VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        nullptr,
        flags,
        queue.family()
    };

    VkCommandPool handle;

    if (auto result = vkCreateCommandPool(device.handle(), &createInfo, nullptr, &handle); result != VK_SUCCESS)
        throw std::runtime_error(fmt::format("failed to create a command buffer: {0:#x}\n"s, result));

    else return handle;

    return { };
}

