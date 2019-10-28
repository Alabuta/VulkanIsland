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

        std::optional<VkAttachmentReference> depth_stencil_attachment;

        std::vector<std::uint32_t> preserve_attachments;
    };
}

namespace graphics
{
    std::shared_ptr<graphics::render_pass>
    render_pass_manager::create_render_pass(std::vector<graphics::attachment_description> const &attachment_descriptions,
                                            std::vector<graphics::subpass_description> const &subpass_descriptions,
                                            std::vector<graphics::subpass_dependency> const &subpass_dependencies)
    {
        auto const subpass_count = std::size(subpass_descriptions);

        std::vector<subpass_invariant> subpass_invariants(subpass_count);

        std::vector<VkAttachmentDescription> attachments;

        for (auto &&description : attachment_descriptions) {
            attachments.push_back(VkAttachmentDescription{
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

        std::vector<VkSubpassDependency> dependencies;

        for (auto &&dependency : subpass_dependencies) {
            dependencies.push_back(VkSubpassDependency{
                dependency.source_index ? *dependency.source_index : 0,
                dependency.destination_index ? *dependency.destination_index : 0,
                convert_to::vulkan(dependency.source_stage),
                convert_to::vulkan(dependency.destination_stage),
                convert_to::vulkan(dependency.source_access),
                convert_to::vulkan(dependency.destination_access),
                0
            });
        }

        std::vector<VkSubpassDescription> subpasses;

        std::size_t subpass_index = 0;

        for (auto &&description : subpass_descriptions) {
            auto &&input_attachments = subpass_invariants.at(subpass_index).input_attachments;

            for (auto &&attachment : description.input_attachments) {
                input_attachments.push_back({
                    attachment.attachment_index, convert_to::vulkan(attachment.subpass_layout)
                });
            }

            auto &&color_attachments = subpass_invariants.at(subpass_index).color_attachments;

            for (auto &&attachment : description.color_attachments) {
                color_attachments.push_back({
                    attachment.attachment_index, convert_to::vulkan(attachment.subpass_layout)
                });
            }

            auto &&depth_stencil_attachment = subpass_invariants.at(subpass_index).depth_stencil_attachment;

            if (description.depth_stencil_attachment) {
                depth_stencil_attachment = VkAttachmentReference{
                    description.depth_stencil_attachment->attachment_index,
                    convert_to::vulkan(description.depth_stencil_attachment->subpass_layout)
                };
            }

            auto &&resolve_attachments = subpass_invariants.at(subpass_index).resolve_attachments;

            for (auto &&attachment : description.resolve_attachments) {
                resolve_attachments.push_back({
                    attachment.attachment_index, convert_to::vulkan(attachment.subpass_layout)
                });
            }

            auto &&preserve_attachments = subpass_invariants.at(subpass_index).preserve_attachments;
            preserve_attachments = description.preserve_attachments;

            subpasses.push_back(VkSubpassDescription{
                0,
                VK_PIPELINE_BIND_POINT_GRAPHICS,
                static_cast<std::uint32_t>(std::size(input_attachments)), std::data(input_attachments),
                static_cast<std::uint32_t>(std::size(color_attachments)), std::data(color_attachments),
                std::data(resolve_attachments),
                depth_stencil_attachment ? &depth_stencil_attachment.value() : nullptr,
                static_cast<std::uint32_t>(std::size(preserve_attachments)), std::data(preserve_attachments)
            });

            ++subpass_index;
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
            static_cast<std::uint32_t>(std::size(dependencies)), std::data(dependencies),
        };

        std::shared_ptr<graphics::render_pass> render_pass;

        VkRenderPass handle;

        if (auto result = vkCreateRenderPass(device_.handle(), &create_info, nullptr, &handle); result != VK_SUCCESS)
            throw std::runtime_error(fmt::format("failed to create render pass: {0:#x}\n"s, result));

        else render_pass.reset(new graphics::render_pass{handle}, [this] (graphics::render_pass *ptr_render_pass)
        {
            vkDestroyRenderPass(device_.handle(), ptr_render_pass->handle(), nullptr);

            delete ptr_render_pass;
        });

        return render_pass;
    }
}
