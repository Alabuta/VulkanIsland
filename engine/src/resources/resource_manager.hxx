#pragma once

#include <unordered_map>
#include <memory>
#include <span>
#include <set>

#include <fmt/format.h>

#include "main.hxx"
#include "utility/mpl.hxx"

#include "vulkan/device.hxx"

#include "renderer/config.hxx"

#include "graphics/render_pass.hxx"
#include "graphics/vertex.hxx"

#include "memory_manager.hxx"


namespace resource
{
    class buffer;

    class image;
    class image_view;
    class sampler;

    class staging_buffer;

    class vertex_buffer;
    class index_buffer;

    class framebuffer;

    class semaphore;
    class fence;

    template<class T>
    struct hash;
}

namespace resource
{
    class resource_manager final {
    public:

        resource_manager(vulkan::device const &device, render::config const &config, resource::memory_manager &memory_manager);

        [[nodiscard]] std::shared_ptr<resource::buffer>
        create_buffer(std::size_t size_bytes, graphics::BUFFER_USAGE usage, graphics::MEMORY_PROPERTY_TYPE memory_property_types, graphics::RESOURCE_SHARING_MODE sharing_mode) const;

        [[nodiscard]] std::shared_ptr<resource::staging_buffer>
        create_staging_buffer(std::size_t size_bytes) const;

        [[nodiscard]] std::shared_ptr<resource::image>
        create_image(graphics::IMAGE_TYPE type, graphics::FORMAT format, render::extent extent, std::uint32_t mip_levels, std::uint32_t samples_count,
                     graphics::IMAGE_TILING tiling, graphics::IMAGE_USAGE usage_flags, graphics::MEMORY_PROPERTY_TYPE memory_property_types) const;

        [[nodiscard]] std::shared_ptr<resource::image_view>
        create_image_view(std::shared_ptr<resource::image> image, graphics::IMAGE_VIEW_TYPE view_type, graphics::IMAGE_ASPECT image_aspect);

        [[nodiscard]] std::shared_ptr<resource::sampler>
        create_image_sampler(graphics::TEXTURE_FILTER min_filter, graphics::TEXTURE_FILTER mag_filter, graphics::TEXTURE_MIPMAP_MODE mipmap_mode,
                             /*float max_anisotropy, */float min_lod, float max_lod);
        
        [[nodiscard]] std::shared_ptr<resource::framebuffer>
        create_framebuffer(std::shared_ptr<graphics::render_pass> render_pass, render::extent extent,
                           std::vector<std::shared_ptr<resource::image_view>> const &attachments);

        [[nodiscard]] std::shared_ptr<resource::semaphore> create_semaphore();
        [[nodiscard]] std::shared_ptr<resource::fence> create_fence(bool create_signaled);

        [[nodiscard]] std::shared_ptr<resource::vertex_buffer>
        stage_vertex_data(graphics::BUFFER_USAGE usage_flags, graphics::vertex_layout const &layout, std::shared_ptr<resource::staging_buffer> staging_buffer, VkCommandPool command_pool);

        [[nodiscard]] std::shared_ptr<resource::index_buffer>
        stage_index_data(graphics::INDEX_TYPE index_type, std::shared_ptr<resource::staging_buffer> staging_buffer, VkCommandPool command_pool);

        [[nodiscard]] std::shared_ptr<resource::image>
        stage_image_data(graphics::IMAGE_TYPE type, graphics::FORMAT format, render::extent extent, graphics::IMAGE_TILING tiling, std::uint32_t mip_levels, std::uint32_t samples_count,
                         std::shared_ptr<resource::staging_buffer> staging_buffer, VkCommandPool command_pool);

    private:

        static std::array<graphics::INDEX_TYPE, 2> constexpr kSUPPORTED_INDEX_FORMATS{graphics::INDEX_TYPE::UINT_16, graphics::INDEX_TYPE::UINT_32};
        static std::array<graphics::FORMAT, 3> constexpr kSUPPORTED_IMAGE_FORMATS{graphics::FORMAT::R8_SRGB, graphics::FORMAT::RG8_SRGB, graphics::FORMAT::RGBA8_SRGB}; // :TODO: replace by run-time acquired list

        // :TODO: consider the config file for following constants.
        static std::size_t constexpr kVERTEX_BUFFER_FIXED_SIZE{0x800'0000}; // 128 MB
        static std::size_t constexpr kINDEX_BUFFER_FIXED_SIZE{0x800'0000}; // 128 MB
        static std::size_t constexpr kIMAGE_BUFFER_FIXED_SIZE{0x800'0000}; // 128 MB

        vulkan::device const &device_;
        render::config const &config_;

        resource::memory_manager &memory_manager_;

        struct resource_deleter;
        std::shared_ptr<resource_deleter> resource_deleter_;

        class staging_buffer_pool;
        std::shared_ptr<staging_buffer_pool> staging_buffer_pool_;

        template<class T>
        struct buffer_set_comparator final {
            using is_transparent = void;

            bool operator() (std::shared_ptr<T> const &lhs, std::shared_ptr<T> const &rhs) const;

            template<class S> requires std::is_unsigned_v<S>
            bool operator() (std::shared_ptr<T> const &buffer, S size_bytes) const;

            template<class S> requires std::is_unsigned_v<S>
            bool operator() (S size_bytes, std::shared_ptr<T> const &buffer) const;
        };

        struct vertex_buffer_key final {
            graphics::vertex_layout vertex_layout;
            graphics::BUFFER_USAGE usage_flags;

            template<class T> requires std::same_as<std::remove_cvref_t<T>, vertex_buffer_key>
            auto constexpr operator== (T &&rhs) const
            {
                return vertex_layout == rhs.vertex_layout && usage_flags == rhs.usage_flags;
            }
        };

        struct vertex_buffer_key_hash {
            std::size_t operator() (vertex_buffer_key const &key) const
            {
                std::size_t seed = 0;

                boost::hash_combine(seed, graphics::hash<graphics::vertex_layout>{}(key.vertex_layout));
                boost::hash_combine(seed, static_cast<std::size_t>(key.usage_flags));

                return seed;
            }
        };

        using vertex_buffer_set = std::multiset<std::shared_ptr<resource::vertex_buffer>, buffer_set_comparator<resource::vertex_buffer>>;
        using index_buffer_set = std::multiset<std::shared_ptr<resource::index_buffer>, buffer_set_comparator<resource::index_buffer>>;

        std::unordered_map<vertex_buffer_key, vertex_buffer_set, vertex_buffer_key_hash> vertex_buffers_;
        std::unordered_map<graphics::INDEX_TYPE, index_buffer_set> index_buffers_;

        std::multiset<std::shared_ptr<resource::image>, buffer_set_comparator<resource::image>> image_buffers_;
    };
}


[[nodiscard]] std::shared_ptr<resource::buffer> create_uniform_buffer(resource::resource_manager &resource_manager, std::size_t size);
[[nodiscard]] std::shared_ptr<resource::buffer> create_coherent_storage_buffer(resource::resource_manager &resource_manager, std::size_t size);
[[nodiscard]] std::shared_ptr<resource::buffer> create_storage_buffer(resource::resource_manager &resource_manager, std::size_t size);
