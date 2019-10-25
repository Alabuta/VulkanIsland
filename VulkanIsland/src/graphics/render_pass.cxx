#include <algorithm>
#include <string>
using namespace std::string_literals;

#include <fmt/format.h>

#include "graphics_api.hxx"
#include "render_pass.hxx"


namespace
{
    struct subpass_invariant final {
        std::vector<VkAttachmentReference> input_attachments;
        std::vector<VkAttachmentReference> color_attachments;
        std::vector<VkAttachmentReference> resolve_attachments;

        VkAttachmentReference depth_stencil_attachments;

        std::vector<std::uint32_t> preserve_attachments;
    };
}

namespace graphics
{
    std::shared_ptr<graphics::render_pass>
    render_pass_manager::create_render_pass(std::vector<graphics::attachment_description> const &attachment_descriptions,
                                            std::vector<graphics::subpass_description> const &subpass_descriptions)
    {
        std::vector<VkAttachmentDescription> attachments;

        for (auto &&description : attachment_descriptions) {
            attachments.push_back({
                0, //VK_ATTACHMENT_DESCRIPTION_MAY_ALIAS_BIT,
                convert_to::vulkan(description.format),
                convert_to::vulkan(description.samples_count),
                convert_to::vulkan(description.load_op),
                convert_to::vulkan(description.store_op),
                convert_to::vulkan(graphics::ATTACHMENT_LOAD_TREATMENT::DONT_CARE),
                convert_to::vulkan(graphics::ATTACHMENT_STORE_TREATMENT::DONT_CARE),
                convert_to::vulkan(description.initial_layout),
                convert_to::vulkan(description.final_layout)
            });
        }

        std::vector<subpass_invariant> subpass_invariants;

        std::vector<VkSubpassDescription> subpasses;

        for (auto &&description : subpass_descriptions) {
            std::vector<VkAttachmentReference> input_attachments;

            for (auto &&attachment : description.input_attachments) {
                input_attachments.push_back({
                    attachment.attachment_index, convert_to::vulkan(attachment.subpass_layout)
                });
            }

            std::vector<VkAttachmentReference> color_attachments;

            for (auto &&attachment : description.color_attachments) {
                color_attachments.push_back({
                    attachment.attachment_index, convert_to::vulkan(attachment.subpass_layout)
                });
            }

            VkAttachmentReference const depth_stencil_attachments{
                description.depth_stencil_attachment.attachment_index,
                convert_to::vulkan(description.depth_stencil_attachment.subpass_layout)
            };

            // TODO:: complete resolve attachments.
            std::vector<VkAttachmentReference> resolve_attachments;

            std::vector<std::uint32_t> preserve_attachments;

            subpasses.push_back(VkSubpassDescription{
                0,
                VK_PIPELINE_BIND_POINT_GRAPHICS,
                static_cast<std::uint32_t>(std::size(input_attachments)), std::data(input_attachments),
                static_cast<std::uint32_t>(std::size(color_attachments)), std::data(color_attachments),
                std::data(resolve_attachments),
                &depth_stencil_attachments,
                static_cast<std::uint32_t>(std::size(preserve_attachments)), std::data(preserve_attachments)
            });

            subpass_invariants.push_back({
                std::move(input_attachments),
                std::move(color_attachments),
                std::move(resolve_attachments),
                std::move(depth_stencil_attachments),
                std::move(preserve_attachments)
            });
        }

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

        VkRenderPassCreateInfo const create_info{
            VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
            nullptr,
            0,
            static_cast<std::uint32_t>(std::size(attachments)), std::data(attachments),
            static_cast<std::uint32_t>(std::size(subpasses)), std::data(subpasses),
            //1, &subpassDependency
        };

        VkRenderPass handle;

        if (auto result = vkCreateRenderPass(device_.handle(), &create_info, nullptr, &handle); result != VK_SUCCESS)
            throw std::runtime_error(fmt::format("failed to create render pass: {0:#x}\n"s, result));

        return std::make_shared<graphics::render_pass>(handle);
    }
}
