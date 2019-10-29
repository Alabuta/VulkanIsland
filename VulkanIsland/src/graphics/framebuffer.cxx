#include <fmt/format.h>

#include "framebuffer.hxx"


namespace resource
{
    [[nodiscard]] std::shared_ptr<resource::framebuffer>
    framebuffer_manager::create_framebuffer(renderer::extent extent, std::shared_ptr<graphics::render_pass> render_pass,
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

        VkFramebuffer handle;

        if (auto result = vkCreateFramebuffer(device.handle(), &create_info, nullptr, &handle); result != VK_SUCCESS)
            throw std::runtime_error(fmt::format("failed to create a framebuffer: {0:#x}\n"s, result));

        return std::make_shared<resource::framebuffer>(handle);
    }
}

namespace graphics
{
    std::size_t hash<resource::framebuffer_invariant>::operator() (resource::framebuffer const &framebuffer_invariant) const
    {
        std::size_t seed = 0;

        boost::hash_combine(seed, framebuffer_invariant.width);
        boost::hash_combine(seed, framebuffer_invariant.height);

        // TODO:: implement
    #if NOT_YET_IMPLEMENTED
        graphics::hash<graphics::render_pass> constexpr render_pass_hasher;
        boost::hash_combine(seed, render_pass_hasher(*framebuffer.render_pass));

        graphics::hash<VulkanImage> constexpr image_hasher;

        for (auto &&attachment : framebuffer.attachments)
            boost::hash_combine(seed, image_hasher(*attachment));
    #endif

        return seed;
    }
}
