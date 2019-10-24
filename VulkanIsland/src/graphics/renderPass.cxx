#include <cmath>

#include "graphics/graphics_api.hxx"
#include "renderPass.hxx"


std::optional<VkRenderPass>
CreateRenderPass(vulkan::device const &device, VulkanSwapchain const &swapchain) noexcept
{
    auto &&device_limits = device.device_limits();

    auto samples_count_bits = std::min(device_limits.framebuffer_color_sample_counts, device_limits.framebuffer_depth_sample_counts);

    VkAttachmentDescription const colorAttachment{
        0, //VK_ATTACHMENT_DESCRIPTION_MAY_ALIAS_BIT,
        convert_to::vulkan(swapchain.format),
        convert_to::vulkan(samples_count_bits),
        VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE,
        VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE,
        VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    };

    VkAttachmentReference constexpr colorAttachmentReference{
        0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    };

    VkAttachmentDescription const depthAttachement{
        0, //VK_ATTACHMENT_DESCRIPTION_MAY_ALIAS_BIT,
        convert_to::vulkan(swapchain.depthTexture->image->format()),
        convert_to::vulkan(samples_count_bits),
        VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_DONT_CARE,
        VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE,
        VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
    };

    VkAttachmentReference constexpr depthAttachementReference{
        1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
    };

    VkAttachmentDescription const colorAttachmentResolve{
        0,
        convert_to::vulkan(swapchain.format),
        VK_SAMPLE_COUNT_1_BIT,
        VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_STORE,
        VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE,
        VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
    };

    VkAttachmentReference constexpr colorAttachmentResolveReference{
        2, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    };

    VkSubpassDescription const subpassDescription{
        0,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        0, nullptr,
        1, &colorAttachmentReference,
        &colorAttachmentResolveReference,
        &depthAttachementReference,
        0, nullptr
    };

    VkSubpassDependency constexpr subpassDependency{
        VK_SUBPASS_EXTERNAL, 0,
        // .srcStageMask needs to be a part of pWaitDstStageMask in the WSI semaphore.
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        0, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        0
    };

    auto const attachments = std::array{ colorAttachment, depthAttachement, colorAttachmentResolve };

#if NOT_YET_IMPLEMENTED
    VkInputAttachmentAspectReference const depthAttachmentAspectReference{
        0, 0, VK_IMAGE_ASPECT_DEPTH_BIT
    };

    VkRenderPassInputAttachmentAspectCreateInfo const depthAttachmentAspect{
        VK_STRUCTURE_TYPE_RENDER_PASS_INPUT_ATTACHMENT_ASPECT_CREATE_INFO,
        nullptr,
        1, &depthAttachmentAspectReference
    };
#endif

    VkRenderPassCreateInfo const renderPassCreateInfo{
        VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        nullptr,
        0,
        static_cast<std::uint32_t>(std::size(attachments)), std::data(attachments),
        1, &subpassDescription,
        1, &subpassDependency
    };

    VkRenderPass handle;

    if (auto result = vkCreateRenderPass(device.handle(), &renderPassCreateInfo, nullptr, &handle); result != VK_SUCCESS) {
        std::cerr << "failed to create render pass: "s << result << '\n';
        return { };
    }

    return handle;
}
