#pragma once

#include <exception>
#include <iostream>
#include <string>
using namespace std::string_literals;

#include <fmt/format.h>

#include "main.hxx"
#include "vulkan/device.hxx"
#include "resources/image.hxx"
#include "graphics/graphics.hxx"
#include "graphics/graphics_api.hxx"


[[nodiscard]] VkCommandBuffer begin_single_time_command(vulkan::device const &device, VkCommandPool command_pool);

void end_single_time_command(VkCommandBuffer command_buffer);

template<class Q>
requires std::is_base_of_v<graphics::queue, std::remove_cvref_t<Q>>
void submit_and_free_single_time_command_buffer(vulkan::device const &device, Q &queue, VkCommandPool command_pool, VkCommandBuffer &command_buffer)
{
    VkSubmitInfo const submit_info{
        VK_STRUCTURE_TYPE_SUBMIT_INFO,
        nullptr,
        0, nullptr,
        nullptr,
        1, &command_buffer,
        0, nullptr,
    };

    if (auto result = vkQueueSubmit(queue.handle(), 1, &submit_info, VK_NULL_HANDLE); result != VK_SUCCESS)
        throw std::runtime_error(fmt::format("failed to submit command buffer: {0:#x}"s, result));

    vkQueueWaitIdle(queue.handle());

    vkFreeCommandBuffers(device.handle(), command_pool, 1, &command_buffer);
}


template<mpl::container R>
void copy_buffer_to_buffer(vulkan::device const &device, graphics::transfer_queue const &queue, VkBuffer src, VkBuffer dst, R &&copy_region, VkCommandPool command_pool)
{
    static_assert(std::is_same_v<typename std::remove_cvref_t<R>::value_type, VkBufferCopy>, "'copyRegion' argument does not contain 'VkBufferCopy' elements");

    auto command_buffer = begin_single_time_command(device, command_pool);

    vkCmdCopyBuffer(command_buffer, src, dst, static_cast<std::uint32_t>(std::size(copy_region)), std::data(copy_region));

    end_single_time_command(command_buffer);

    submit_and_free_single_time_command_buffer(device, queue, command_pool, command_buffer);
}

void copy_buffer_to_image(vulkan::device const &device, graphics::transfer_queue const &queue, VkBuffer src, VkImage dst, renderer::extent extent, VkCommandPool command_pool);

template<class Q>
requires std::is_base_of_v<graphics::queue, std::remove_cvref_t<Q>>
void generate_mip_maps(vulkan::device const &device, Q &queue, resource::image const &image, VkCommandPool command_pool) noexcept
{
    auto command_buffer = begin_single_time_command(device, command_pool);

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

        vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

        VkImageBlit const imageBlit{
            { VK_IMAGE_ASPECT_COLOR_BIT, i - 1, 0, 1 },
            {{ 0, 0, 0 }, { static_cast<std::int32_t>(width), static_cast<std::int32_t>(height), 1 }},
            { VK_IMAGE_ASPECT_COLOR_BIT, i, 0, 1 },
            {{ 0, 0, 0 }, { static_cast<std::int32_t>(width / 2), static_cast<std::int32_t>(height / 2), 1 }}
        };

        vkCmdBlitImage(command_buffer, image.handle(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, image.handle(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imageBlit, VK_FILTER_LINEAR);

        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

        if (width > 1) width /= 2;
        if (height > 1) height /= 2;
    }

    barrier.subresourceRange.baseMipLevel = image.mip_levels() - 1;
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

    end_single_time_command(command_buffer);

    submit_and_free_single_time_command_buffer(device, queue, command_pool, command_buffer);
}

template<class Q>
requires std::is_base_of_v<graphics::queue, std::remove_cvref_t<Q>>
void image_layout_transition(vulkan::device const &device, Q &queue, resource::image const &image,
                             graphics::IMAGE_LAYOUT src, graphics::IMAGE_LAYOUT dst, VkCommandPool command_pool)
{
    VkImageMemoryBarrier barrier{
        VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        nullptr,
        0, 0,
        convert_to::vulkan(src), convert_to::vulkan(dst),
        VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
        image.handle(),
        { VK_IMAGE_ASPECT_COLOR_BIT, 0, image.mip_levels(), 0, 1 }
    };

    if (dst == graphics::IMAGE_LAYOUT::DEPTH_STENCIL_ATTACHMENT) {
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

        if (image.format() == graphics::FORMAT::D32_SFLOAT_S8_UINT || image.format() == graphics::FORMAT::D24_UNORM_S8_UINT)
            barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
    }

    graphics::PIPELINE_STAGE src_stage_flags, dst_stage_flags;

    if (src == graphics::IMAGE_LAYOUT::UNDEFINED && dst == graphics::IMAGE_LAYOUT::TRANSFER_DESTINATION) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        src_stage_flags = graphics::PIPELINE_STAGE::TOP_OF_PIPE;
        dst_stage_flags = graphics::PIPELINE_STAGE::TRANSFER;
    }

    else if (src == graphics::IMAGE_LAYOUT::TRANSFER_DESTINATION && dst == graphics::IMAGE_LAYOUT::SHADER_READ_ONLY) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        src_stage_flags = graphics::PIPELINE_STAGE::TRANSFER;
        dst_stage_flags = graphics::PIPELINE_STAGE::FRAGMENT_SHADER;
    }

    else if (src == graphics::IMAGE_LAYOUT::UNDEFINED && dst == graphics::IMAGE_LAYOUT::DEPTH_STENCIL_ATTACHMENT) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        src_stage_flags = graphics::PIPELINE_STAGE::TOP_OF_PIPE;
        dst_stage_flags = graphics::PIPELINE_STAGE::EARLY_FRAGMENT_TESTS;
    }

    else if (src == graphics::IMAGE_LAYOUT::UNDEFINED && dst == graphics::IMAGE_LAYOUT::COLOR_ATTACHMENT) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        src_stage_flags = graphics::PIPELINE_STAGE::TOP_OF_PIPE;
        dst_stage_flags = graphics::PIPELINE_STAGE::COLOR_ATTACHMENT_OUTPUT;
    }

    else throw std::runtime_error("unsupported layout transition"s);

    auto command_buffer = begin_single_time_command(device, command_pool);

    vkCmdPipelineBarrier(command_buffer, convert_to::vulkan(src_stage_flags), convert_to::vulkan(dst_stage_flags), 0, 0, nullptr, 0, nullptr, 1, &barrier);

    end_single_time_command(command_buffer);

    submit_and_free_single_time_command_buffer(device, queue, command_pool, command_buffer);
}


template<class Q>
requires std::is_base_of_v<graphics::queue, std::remove_cvref_t<Q>>
std::optional<VkCommandPool> create_command_pool(vulkan::device const &device, Q &queue, VkCommandPoolCreateFlags flags)
{
    VkCommandPoolCreateInfo const create_info{
        VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        nullptr,
        flags,
        queue.family()
    };

    VkCommandPool handle;

    if (auto result = vkCreateCommandPool(device.handle(), &create_info, nullptr, &handle); result != VK_SUCCESS)
        throw std::runtime_error(fmt::format("failed to create a command buffer: {0:#x}"s, result));

    else return handle;
}

