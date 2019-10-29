#include <fmt/format.h>

#include "image.hxx"
#include "framebuffer.hxx"
#include "resource_manager.hxx"


namespace resource
{
    [[nodiscard]] std::shared_ptr<resource::framebuffer>
    resource_manager::create_framebuffer(renderer::extent extent, std::shared_ptr<graphics::render_pass> render_pass,
                                        std::vector<std::shared_ptr<resource::image_view>> const &attachments)
    {
        std::vector<VkImageView> views;

        std::transform(std::cbegin(attachments), std::cend(attachments), std::back_inserter(views), [] (auto &&attachment)
        {
            return attachment->handle();
        });

        VkFramebufferCreateInfo const create_info{
            VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            nullptr, 0,
            render_pass->handle(),
            static_cast<std::uint32_t>(std::size(views)), std::data(views),
            extent.width, extent.height, 1
        };

        std::shared_ptr<resource::framebuffer> framebuffer;

        VkFramebuffer handle;

        if (auto result = vkCreateFramebuffer(device_.handle(), &create_info, nullptr, &handle); result != VK_SUCCESS)
            throw std::runtime_error(fmt::format("failed to create a framebuffer: {0:#x}\n"s, result));

        else framebuffer.reset(new resource::framebuffer{handle}, [this] (resource::framebuffer *ptr_framebuffer)
        {
            vkDestroyFramebuffer(device_.handle(), ptr_framebuffer->handle(), nullptr);

            delete ptr_framebuffer;
        });

        return framebuffer;
    }
}