#include <unordered_map>
#include <vector>

#include <fmt/format.h>

#include "image.hxx"
#include "framebuffer.hxx"
#include "resource_manager.hxx"


namespace resource
{
    struct resource_map final {
        std::unordered_map<framebuffer_invariant, std::shared_ptr<framebuffer>, hash<framebuffer_invariant>> framebuffers;
    };
}

namespace resource
{
    resource_manager::resource_manager(vulkan::device const &device, renderer::config const &config)
        : device_{device}, config_{config}, resource_map_{std::make_shared<resource::resource_map>()} { };

    std::shared_ptr<resource::image>
    resource_manager::create_image(graphics::FORMAT format, renderer::extent extent, std::uint32_t mip_levels, std::uint32_t samples_count,
                                   graphics::IMAGE_TILING tiling, graphics::IMAGE_USAGE usage_flags, graphics::MEMORY_PROPERTY_TYPE memory_property_type)
    {
        return std::shared_ptr<resource::image>();
    }

    std::shared_ptr<resource::image_view>
    resource_manager::create_image_view(std::shared_ptr<resource::image> image, graphics::IMAGE_VIEW_TYPE view_type, graphics::IMAGE_ASPECT image_aspect)
    {
        return std::shared_ptr<resource::image_view>();
    }

    std::shared_ptr<resource::sampler>
    resource_manager::create_image_sampler(graphics::TEXTURE_FILTER min_filter, graphics::TEXTURE_FILTER mag_filter, graphics::TEXTURE_MIPMAP_MODE mipmap_mode,
                                           float max_anisotropy, std::uint32_t min_lod, std::uint32_t max_lod)
    {
        return std::shared_ptr<resource::sampler>();
    }

    [[nodiscard]] std::shared_ptr<resource::framebuffer>
    resource_manager::create_framebuffer(std::shared_ptr<graphics::render_pass> render_pass, renderer::extent extent,
                                         std::vector<std::shared_ptr<resource::image_view>> const &attachments)
    {
        resource::framebuffer_invariant invariant{
            extent, render_pass, attachments
        };

        if (resource_map_->framebuffers.contains(invariant))
            return resource_map_->framebuffers.at(invariant);

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

        resource_map_->framebuffers.emplace(std::move(invariant), framebuffer);

        return framebuffer;
    }
}
