#include "utility/exceptions.hxx"
#include "render_flow.hxx"


namespace graphics
{
    render_pipeline_manager::render_pipeline_manager(vulkan::device const &device,
                                                     std::shared_ptr<resource::resource_manager> resource_manager)
        : device_{device}, resource_manager_{resource_manager}
    {
        render_pass_manager_ = std::make_unique<graphics::render_pass_manager>(device);
    }
    
    graphics::render_pipeline
    render_pipeline_manager::create_render_flow(renderer::swapchain const &swapchain, std::vector<graphics::render_pipeline_node> const &)
    {
        return graphics::render_pipeline();
    }
}

std::vector<graphics::attachment>
create_attachments(vulkan::device const &device, renderer::config const &renderer_config, resource::resource_manager &resource_manager, renderer::swapchain const &swapchain)
{
    auto constexpr mip_levels = 1u;
    auto constexpr image_type = graphics::IMAGE_TYPE::TYPE_2D;
    auto constexpr view_type = graphics::IMAGE_VIEW_TYPE::TYPE_2D;

    std::vector<graphics::attachment> attachments;

    auto samples_count = renderer_config.framebuffer_sample_counts;

    auto extent = swapchain.extent();

    {
        /* | graphics::IMAGE_USAGE::TRANSFER_DESTINATION*/
        auto constexpr usage_flags = graphics::IMAGE_USAGE::TRANSIENT_ATTACHMENT | graphics::IMAGE_USAGE::COLOR_ATTACHMENT;
        auto constexpr property_flags = graphics::MEMORY_PROPERTY_TYPE::DEVICE_LOCAL /*| VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT*/;
        auto constexpr tiling = graphics::IMAGE_TILING::OPTIMAL;
        auto constexpr aspect_flags = graphics::IMAGE_ASPECT::COLOR_BIT;

        auto format = swapchain.surface_format().format;

        auto image = resource_manager.create_image(image_type, format, extent, mip_levels, samples_count, tiling, usage_flags, property_flags);

        if (image == nullptr)
            throw graphics::exception("failed to create image for the color attachment"s);

        auto image_view = resource_manager.create_image_view(image, view_type, aspect_flags);

        if (image_view == nullptr)
            throw graphics::exception("failed to create image view for the color attachment"s);

        attachments.push_back(graphics::color_attachment{format, tiling, mip_levels, samples_count, image, image_view});
    }

    {
        auto constexpr usage_flags = graphics::IMAGE_USAGE::TRANSIENT_ATTACHMENT | graphics::IMAGE_USAGE::DEPTH_STENCIL_ATTACHMENT;
        auto constexpr property_flags = graphics::MEMORY_PROPERTY_TYPE::DEVICE_LOCAL /*| VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT*/;
        auto constexpr tiling = graphics::IMAGE_TILING::OPTIMAL;
        auto constexpr aspect_flags = graphics::IMAGE_ASPECT::DEPTH_BIT;

        auto format = find_supported_image_format(
            device,
            {graphics::FORMAT::D32_SFLOAT, graphics::FORMAT::D32_SFLOAT_S8_UINT, graphics::FORMAT::D24_UNORM_S8_UINT},
            tiling,
            graphics::FORMAT_FEATURE::DEPTH_STENCIL_ATTACHMENT
        );

        if (!format)
            throw graphics::exception("failed to find supported depth format"s);

        auto image = resource_manager.create_image(image_type, *format, extent, mip_levels, samples_count, tiling, usage_flags, property_flags);

        if (image == nullptr)
            throw graphics::exception("failed to create image for the depth attachment"s);

        auto image_view = resource_manager.create_image_view(image, view_type, aspect_flags);

        if (image_view == nullptr)
            throw graphics::exception("failed to create image view for the depth attachment"s);

        attachments.push_back(graphics::depth_attachment{*format, tiling, mip_levels, samples_count, image, image_view});
    }

    return attachments;
}

std::vector<graphics::attachment_description>
create_attachment_descriptions(std::vector<graphics::attachment> const &attachments)
{
    auto [color_attachment_format, color_attachment_samples_count] = std::visit([] (auto &&attachment)
    {
        return std::pair{attachment.format, attachment.samples_count};

    }, attachments.at(0));

    auto [depth_attachment_format, depth_attachment_samples_count] = std::visit([] (auto &&attachment)
    {
        return std::pair{attachment.format, attachment.samples_count};

    }, attachments.at(1));

    return std::vector{
        graphics::attachment_description{
            color_attachment_format,
            color_attachment_samples_count,
            graphics::ATTACHMENT_LOAD_TREATMENT::CLEAR,
            graphics::ATTACHMENT_STORE_TREATMENT::DONT_CARE,
            graphics::IMAGE_LAYOUT::UNDEFINED,
            graphics::IMAGE_LAYOUT::COLOR_ATTACHMENT
        },
        graphics::attachment_description{
            depth_attachment_format,
            depth_attachment_samples_count,
            graphics::ATTACHMENT_LOAD_TREATMENT::CLEAR,
            graphics::ATTACHMENT_STORE_TREATMENT::DONT_CARE,
            graphics::IMAGE_LAYOUT::UNDEFINED,
            graphics::IMAGE_LAYOUT::DEPTH_STENCIL_ATTACHMENT
        }
    };
}

std::shared_ptr<graphics::render_pass>
create_render_pass(graphics::render_pass_manager &render_pass_manager, renderer::surface_format surface_format,
                   std::vector<graphics::attachment_description> attachment_descriptions)
{
    attachment_descriptions.push_back(
        graphics::attachment_description{
            surface_format.format,
            1,
            graphics::ATTACHMENT_LOAD_TREATMENT::DONT_CARE,
            graphics::ATTACHMENT_STORE_TREATMENT::STORE,
            graphics::IMAGE_LAYOUT::UNDEFINED,
            graphics::IMAGE_LAYOUT::PRESENT_SOURCE
        }
    );
    
    return render_pass_manager.create_render_pass(
        attachment_descriptions,
        std::vector{
            graphics::subpass_description{
                { },
                {
                    graphics::attachment_reference{
                        0, 0, graphics::IMAGE_LAYOUT::COLOR_ATTACHMENT
                    }
                },
                {
                    graphics::attachment_reference{
                        0, 2, graphics::IMAGE_LAYOUT::COLOR_ATTACHMENT
                    }
                },
                graphics::attachment_reference{
                    0, 1, graphics::IMAGE_LAYOUT::DEPTH_STENCIL_ATTACHMENT
                },
                { }
            }
        },
        std::vector{
            graphics::subpass_dependency{
                std::nullopt, 0,
                graphics::PIPELINE_STAGE::COLOR_ATTACHMENT_OUTPUT,
                graphics::PIPELINE_STAGE::COLOR_ATTACHMENT_OUTPUT,
                std::nullopt,
                graphics::MEMORY_ACCESS_TYPE::COLOR_ATTACHMENT_READ | graphics::MEMORY_ACCESS_TYPE::COLOR_ATTACHMENT_WRITE
            }
        }
    );
}

std::vector<std::shared_ptr<resource::framebuffer>>
create_framebuffers(resource::resource_manager &resource_manager, renderer::swapchain const &swapchain,
                    std::shared_ptr<graphics::render_pass> render_pass, std::vector<graphics::attachment> const &attachments)
{
    std::vector<std::shared_ptr<resource::framebuffer>> framebuffers;

    auto &&swapchain_views = swapchain.image_views();

    std::vector<std::shared_ptr<resource::image_view>> image_views;

    for (auto attachment : attachments) {
        std::visit([&image_views] (auto &&attachment)
        {
            image_views.push_back(attachment.image_view);

        }, std::move(attachment));
    }

    for (auto &&swapchain_view : swapchain_views) {
        auto image_views_copy = image_views;
        image_views_copy.push_back(swapchain_view);

        auto framebuffer = resource_manager.create_framebuffer(render_pass, swapchain.extent(), image_views_copy);

        framebuffers.push_back(std::move(framebuffer));
    }

    return framebuffers;
}
