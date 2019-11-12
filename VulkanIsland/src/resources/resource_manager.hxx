#pragma once

#include <memory>

#include "main.hxx"
#include "utility/mpl.hxx"

#include "vulkan/device.hxx"

#include "renderer/config.hxx"

#include "graphics/render_pass.hxx"

#include "memory.hxx"


namespace resource
{
    class vertex_buffer;
    class index_buffer;

    class image;
    class image_view;

    class framebuffer;

    template<class T>
    struct hash;
}

namespace resource
{
    class resource_manager final {
    public:

        resource_manager(vulkan::device const &device, renderer::config const &config, MemoryManager &memory_manager);

        [[nodiscard]] std::shared_ptr<resource::buffer>
        create_buffer(std::size_t size_in_bytes, graphics::BUFFER_USAGE usage, graphics::MEMORY_PROPERTY_TYPE memory_property_type);

        [[nodiscard]] std::shared_ptr<resource::image>
        create_image(graphics::IMAGE_TYPE type, graphics::FORMAT format, renderer::extent extent, std::uint32_t mip_levels, std::uint32_t samples_count,
                     graphics::IMAGE_TILING tiling, graphics::IMAGE_USAGE usage_flags, graphics::MEMORY_PROPERTY_TYPE memory_property_type);

        [[nodiscard]] std::shared_ptr<resource::image_view>
        create_image_view(std::shared_ptr<resource::image> image, graphics::IMAGE_VIEW_TYPE view_type, graphics::IMAGE_ASPECT image_aspect);

        [[nodiscard]] std::shared_ptr<resource::sampler>
        create_image_sampler(graphics::TEXTURE_FILTER min_filter, graphics::TEXTURE_FILTER mag_filter, graphics::TEXTURE_MIPMAP_MODE mipmap_mode,
                             float max_anisotropy, float min_lod, float max_lod);
        
        [[nodiscard]] std::shared_ptr<resource::framebuffer>
        create_framebuffer(std::shared_ptr<graphics::render_pass> render_pass, renderer::extent extent,
                           std::vector<std::shared_ptr<resource::image_view>> const &attachments);

        [[nodiscard]] std::shared_ptr<resource::semaphore> create_semaphore();

        [[nodiscard]] std::shared_ptr<resource::vertex_buffer> create_vertex_buffer(graphics::vertex_layout const &layout, std::size_t size_in_bytes);

        [[nodiscard]] std::shared_ptr<resource::index_buffer> create_index_buffer(std::size_t size_in_bytes);

    private:

        vulkan::device const &device_;
        renderer::config const &config_;

        MemoryManager &memory_manager_;

        std::shared_ptr<struct resource_map> resource_map_;
    };
}
