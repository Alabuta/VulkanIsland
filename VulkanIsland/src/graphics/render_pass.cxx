#include <algorithm>
#include <string>
using namespace std::string_literals;

#include <fmt/format.h>

#include "render_pass.hxx"


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
        
        std::vector<VkAttachmentReference> references;

        for (auto &&description : subpass_descriptions) {
            // references.push_back();
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
            1, &subpassDescription,
            1, &subpassDependency
        };

        VkRenderPass handle;

        if (auto result = vkCreateRenderPass(device.handle(), &create_info, nullptr, &handle); result != VK_SUCCESS)
            throw std::runtime_error(fmt::format("failed to create render pass: {0:#x}\n"s, result));

        return std::make_shared<graphics::render_pass>(handle);
    }
}
