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

#include "memory_manager.hxx"


namespace resource
{
    class buffer;

    class image;
    class image_view;
    class sampler;

    class vertex_buffer;
    class index_buffer;

    class framebuffer;

    class semaphore;

    template<class T>
    struct hash;
}

namespace resource
{
    class resource_manager final {
    public:

        resource_manager(vulkan::device const &device, renderer::config const &config, resource::memory_manager &memory_manager);

        [[nodiscard]] std::shared_ptr<resource::buffer>
        create_buffer(std::size_t size_bytes, graphics::BUFFER_USAGE usage, graphics::MEMORY_PROPERTY_TYPE memory_property_types);

        [[nodiscard]] std::shared_ptr<resource::image>
        create_image(graphics::IMAGE_TYPE type, graphics::FORMAT format, renderer::extent extent, std::uint32_t mip_levels, std::uint32_t samples_count,
                     graphics::IMAGE_TILING tiling, graphics::IMAGE_USAGE usage_flags, graphics::MEMORY_PROPERTY_TYPE memory_property_types);

        [[nodiscard]] std::shared_ptr<resource::image_view>
        create_image_view(std::shared_ptr<resource::image> image, graphics::IMAGE_VIEW_TYPE view_type, graphics::IMAGE_ASPECT image_aspect);

        [[nodiscard]] std::shared_ptr<resource::sampler>
        create_image_sampler(graphics::TEXTURE_FILTER min_filter, graphics::TEXTURE_FILTER mag_filter, graphics::TEXTURE_MIPMAP_MODE mipmap_mode,
                             float max_anisotropy, float min_lod, float max_lod);
        
        [[nodiscard]] std::shared_ptr<resource::framebuffer>
        create_framebuffer(std::shared_ptr<graphics::render_pass> render_pass, renderer::extent extent,
                           std::vector<std::shared_ptr<resource::image_view>> const &attachments);

        [[nodiscard]] std::shared_ptr<resource::semaphore> create_semaphore();

        bool is_vertex_buffer_exist(graphics::vertex_layout const &layout) const noexcept;
        bool is_index_buffer_exist(graphics::FORMAT format) const noexcept;

        [[nodiscard]] std::shared_ptr<resource::vertex_buffer> get_vertex_buffer(graphics::vertex_layout const &layout);
        // [[nodiscard]] std::shared_ptr<resource::index_buffer> create_index_buffer(graphics::FORMAT format);

        [[nodiscard]] std::shared_ptr<resource::vertex_buffer> create_vertex_buffer(graphics::vertex_layout const &layout, std::size_t size_bytes);
        [[nodiscard]] std::shared_ptr<resource::index_buffer> create_index_buffer(graphics::FORMAT format, std::size_t size_bytes);

        [[nodiscard]] auto &vertex_buffers() const noexcept { return vertex_buffers_; }
        [[nodiscard]] auto &index_buffers() const noexcept { return index_buffers_; }

        template<class T> requires mpl::one_of<T, resource::vertex_buffer, resource::index_buffer>
        void stage_buffer_data(std::shared_ptr<T> buffer, std::span<std::byte const> const container) const;

        void transfer_vertex_buffers_data(VkCommandPool command_pool, graphics::transfer_queue const &transfer_queue);
        void transfer_index_buffers_data(VkCommandPool command_pool, graphics::transfer_queue const &transfer_queue);

        [[nodiscard]] std::shared_ptr<resource::vertex_buffer>
        stage_vertex_data(graphics::vertex_layout const &layout, std::span<std::byte const> const container);

    private:

        // :TODO: consider the config file for following constants.
        static std::size_t constexpr kSTAGING_BUFFER_SIZE{0x800'0000}; // 128 MB
        static std::size_t constexpr kVERTEX_BUFFER_FIXED_SIZE{0x800'0000}; // 128 MB
        static std::size_t constexpr kINDEX_BUFFER_FIXED_SIZE{0x800'0000}; // 128 MB

        static std::size_t constexpr kVERTEX_BUFFER_INCREASE_VALUE{4};
        static std::size_t constexpr kINDEX_BUFFER_INCREASE_VALUE{4};

        vulkan::device const &device_;
        renderer::config const &config_;

        resource::memory_manager &memory_manager_;

        std::shared_ptr<struct resource_deleter> resource_deleter_;

        /*std::unordered_map<std::size_t, std::shared_ptr<resource::buffer>> buffers_;
        std::unordered_map<std::size_t, std::shared_ptr<resource::image>> images_;*/

        // TODO:: unordered_miltimap
        std::unordered_map<graphics::vertex_layout, std::shared_ptr<resource::vertex_buffer>, graphics::hash<graphics::vertex_layout>> vertex_buffers_;
        std::unordered_map<graphics::FORMAT, std::shared_ptr<resource::index_buffer>> index_buffers_;


        std::shared_ptr<resource::buffer> staging_buffer_;

        struct vertex_buffer_comparator final {
            //using is_transparent = void;

            template<class L, class R>
            requires mpl::are_same_v<resource::vertex_buffer, L, R>
            bool operator() (std::shared_ptr<L> lhs, std::shared_ptr<R> rhs) const noexcept
            {
                if (lhs.vertex_layout() == rhs.vertex_layout())
                    return lhs.available_size() < rhs.available_size();

                return lhs.vertex_layout() < rhs.vertex_layout();
            }

            /*template<class T, class S>
            requires std::is_same_v<resource::vertex_buffer, std::remove_cvref_t<T>> && std::is_unsigned_v<S>
            bool operator() (T &&buffer, S size_bytes) const noexcept
            {
                return buffer.available_size() < size_bytes;
            }

            template<class S, class T>
            requires std::is_same_v<resource::vertex_buffer, std::remove_cvref_t<T>> && std::is_unsigned_v<S>
            bool operator() (S size_bytes, T &&buffer) const noexcept
            {
                return buffer.available_size() < size_bytes;
            }*/
        };

        std::multiset<std::shared_ptr<resource::vertex_buffer>, resource::resource_manager::vertex_buffer_comparator> vbs_;

        /*struct vertex_buffer_page final {
            std::shared_ptr<resource::buffer> buffer;

            std::size_t available_size{0};

            struct comparator final {
                using is_transparent = void;

                template<class L, class R>
                requires mpl::are_same_v<vertex_buffer_page, std::remove_cvref_t<L>, std::remove_cvref_t<R>>
                bool operator() (L &&lhs, R &&rhs) const noexcept
                {
                    return lhs.available_size < rhs.available_size;
                }

                template<class T, class S>
                requires std::is_same_v<vertex_buffer_page, std::remove_cvref_t<T>> && std::is_unsigned_v<S>
                bool operator() (T &&page, S size_bytes) const noexcept
                {
                    return page.available_size < size_bytes;
                }

                template<class S, class T>
                requires std::is_same_v<vertex_buffer_page, std::remove_cvref_t<T>> && std::is_unsigned_v<S>
                bool operator() (S size_bytes, T &&page) const noexcept
                {
                    return page.available_size < size_bytes;
                }
            };
        };

        std::multiset<vertex_buffer_page, vertex_buffer_page::comparator> vb_pages_;*/
    };

    template<class T> requires mpl::one_of<T, resource::vertex_buffer, resource::index_buffer>
    void resource_manager::stage_buffer_data(std::shared_ptr<T> vertex_buffer, std::span<std::byte const> const container) const
    {
        if (vertex_buffer == nullptr)
            return;

        if (vertex_buffer->available_staging_buffer_size() < container.size_bytes())
            throw resource::not_enough_memory("not enough staging memory for buffer"s);

        // TODO: sparse memory binding
        if (vertex_buffer->available_device_buffer_size() < container.size_bytes())
            throw resource::not_enough_memory("not enough device memory for buffer"s);

        auto &&memory = vertex_buffer->staging_buffer().memory();

        void *ptr;

        if (auto result = vkMapMemory(device_.handle(), memory->handle(), vertex_buffer->staging_memory_offset(), container.size_bytes(), 0, &ptr); result != VK_SUCCESS)
            throw resource::exception(fmt::format("failed to map staging buffer memory: {0:#x}"s, result));

        else {
            std::copy(std::begin(container), std::end(container), reinterpret_cast<std::byte *>(ptr));

            vkUnmapMemory(device_.handle(), memory->handle());

            vertex_buffer->staging_buffer_offset_ += container.size_bytes();
        }
    }
}


[[nodiscard]] std::shared_ptr<resource::buffer> CreateUniformBuffer(resource::resource_manager &resource_manager, std::size_t size);
[[nodiscard]] std::shared_ptr<resource::buffer> create_coherent_storage_buffer(resource::resource_manager &resource_manager, std::size_t size);
[[nodiscard]] std::shared_ptr<resource::buffer> CreateStorageBuffer(resource::resource_manager &resource_manager, std::size_t size);
