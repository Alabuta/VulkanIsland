#include <unordered_map>
#include <vector>
#include <ranges>

#include <string>
using namespace std::string_literals;

#include <fmt/format.h>

#include "utility/exceptions.hxx"
#include "graphics/graphics_api.hxx"
#include "buffer.hxx"
#include "image.hxx"
#include "sync_objects.hxx"
#include "framebuffer.hxx"
#include "renderer/command_buffer.hxx"

#include "resource_manager.hxx"


namespace resource
{
    struct buffer_chunk final {
        buffer_chunk(std::size_t offset, std::size_t size) noexcept : offset{offset}, size{size} { }

        std::size_t offset{0}, size{0};

        struct comparator final {
            using is_transparent = void;

            template<class L, class R>
            requires mpl::are_same_v<buffer_chunk, L, R>
            bool operator() (L lhs, R rhs) const noexcept
            {
                return lhs.size < rhs.size;
            }

            template<class T, class S>
            requires mpl::are_same_v<buffer_chunk, T> && std::is_unsigned_v<S>
            bool operator() (T chunk, S rhs_size) const noexcept
            {
                return chunk.size < rhs_size;
            }
                
            template<class S, class T>
            requires mpl::are_same_v<buffer_chunk, T> && std::is_unsigned_v<S>
            bool operator() (S lhs_size, T chunk) const noexcept
            {
                return chunk.size < lhs_size;
            }
        };
    };

    class resource_manager::staging_buffer_pool final {
    public:

        staging_buffer_pool(vulkan::device const &device, resource::resource_manager &resource_manager);
        ~staging_buffer_pool();

        std::shared_ptr<resource::buffer> buffer() const { return buffer_; }
        std::span<std::byte> total_mapped_range() const noexcept { return total_mapped_range_; }

        std::pair<std::size_t, std::span<std::byte>> allocate_mapped_range(std::size_t size_bytes);
        void unmap_range(std::size_t offset_bytes, std::span<std::byte> mapped_range);

        //void operator() (resource::staging_buffer *resource_ptr);

    private:

        static auto constexpr kBUFFER_USAGE_FLAGS{graphics::BUFFER_USAGE::TRANSFER_SOURCE};
        static auto constexpr kSHARING_MODE{graphics::RESOURCE_SHARING_MODE::EXCLUSIVE};
        static auto constexpr kMEMORY_PROPERTY_TYPES{graphics::MEMORY_PROPERTY_TYPE::HOST_VISIBLE | graphics::MEMORY_PROPERTY_TYPE::HOST_COHERENT};

        static auto constexpr kPOOL_SIZE_BYTES{resource::memory_manager::kPAGE_ALLOCATION_SIZE};

        vulkan::device const &device_;
        resource::resource_manager &resource_manager_;

        std::shared_ptr<resource::buffer> buffer_;
        std::span<std::byte> total_mapped_range_;

        std::multiset<resource::buffer_chunk, resource::buffer_chunk::comparator> available_chunks_;
    };

    resource_manager::staging_buffer_pool::staging_buffer_pool(vulkan::device const &device, resource::resource_manager &resource_manager)
        : device_{device}, resource_manager_{resource_manager}
    {
        buffer_ = resource_manager_.create_buffer(kPOOL_SIZE_BYTES, kBUFFER_USAGE_FLAGS, kMEMORY_PROPERTY_TYPES, kSHARING_MODE);

        if (buffer_ == nullptr)
            throw resource::instantiation_fail(fmt::format("failed to create the staging pool buffer"));

        auto &&memory = buffer_->memory();

        void *mapped_ptr{nullptr};

        if (auto result = vkMapMemory(device_.handle(), memory->handle(), memory->offset(), kPOOL_SIZE_BYTES, 0, &mapped_ptr); result != VK_SUCCESS)
            throw resource::exception(fmt::format("failed to map staging buffer memory: {0:#x}", result));

        total_mapped_range_ = std::span{static_cast<std::byte *>(mapped_ptr), kPOOL_SIZE_BYTES};

        available_chunks_.emplace(0, kPOOL_SIZE_BYTES);
    }

    resource_manager::staging_buffer_pool::~staging_buffer_pool()
    {
        vkUnmapMemory(device_.handle(), buffer_->memory()->handle());
    }

    std::pair<std::size_t, std::span<std::byte>> resource_manager::staging_buffer_pool::allocate_mapped_range(std::size_t size_bytes)
    {
        if (size_bytes > kPOOL_SIZE_BYTES)
            throw resource::not_enough_memory("requested staging buffer allocation size is bigger than staging buffer pool size."s);

        auto it_chunk = available_chunks_.lower_bound(size_bytes);

        if (it_chunk == std::end(available_chunks_))
            throw resource::not_enough_memory("failed to find available staging buffer pool memory chunk."s);

        if (auto node_handle = available_chunks_.extract(it_chunk); node_handle) {
            auto [offset, size] = node_handle.value();

            available_chunks_.emplace(offset + size_bytes, size - size_bytes);
            available_chunks_.erase(resource::buffer_chunk{0, 0});

            return {offset, std::span{std::data(total_mapped_range_) + offset, size_bytes}};
        }

        else throw resource::exception("failed to find staging buffer pool memory chunk for extraction"s);
    }

    void resource_manager::staging_buffer_pool::unmap_range(std::size_t offset_bytes, std::span<std::byte> mapped_range)
    {
        auto size_bytes = std::size(mapped_range);

        auto find_adjacent_chunk = [] (auto begin, auto end, auto it_chunk)
        {
            return std::find_if(begin, end, [it_chunk] (auto &&chunk)
            {
                return chunk.offset + chunk.size == it_chunk->offset || it_chunk->offset + it_chunk->size == chunk.offset;
            });
        };

        auto it_current_chunk = available_chunks_.emplace(offset_bytes, size_bytes);
        auto it_adjacent_chunk = find_adjacent_chunk(std::begin(available_chunks_), std::end(available_chunks_), it_current_chunk);

        while (it_adjacent_chunk != std::end(available_chunks_)) {
            auto [offset_a, size_a] = *it_current_chunk;
            auto [offset_b, size_b] = *it_adjacent_chunk;

            available_chunks_.erase(it_current_chunk);
            available_chunks_.erase(it_adjacent_chunk);

            it_current_chunk = available_chunks_.emplace(std::min(offset_a, offset_b), size_a + size_b);

            it_adjacent_chunk = find_adjacent_chunk(std::begin(available_chunks_), std::end(available_chunks_), it_current_chunk);
        }
    }
}

namespace resource
{
    struct resource_manager::resource_deleter final {
        vulkan::device const &device;
        resource::resource_manager &resource_manager;

        resource_deleter(vulkan::device const &device, resource::resource_manager &resource_manager) noexcept
            : device{device}, resource_manager{resource_manager} { }

        template<class T>
        void operator() (T *resource_ptr) const
        {
            if (resource_ptr == nullptr)
                return;

            if constexpr (std::is_same_v<T, resource::buffer>) {
                vkDestroyBuffer(device.handle(), resource_ptr->handle(), nullptr);

                if (resource_ptr->memory())
                    resource_ptr->memory().reset();
            }

            else if constexpr (std::is_same_v<T, resource::staging_buffer>)
                resource_manager.staging_buffer_pool_->unmap_range(resource_ptr->offset_bytes(), resource_ptr->mapped_range());

            else if constexpr (std::is_same_v<T, resource::image>) {
                vkDestroyImage(device.handle(), resource_ptr->handle(), nullptr);

                if (resource_ptr->memory())
                    resource_ptr->memory().reset();
            }

            else if constexpr (std::is_same_v<T, resource::image_view>)
                vkDestroyImageView(device.handle(), resource_ptr->handle(), nullptr);

            else if constexpr (std::is_same_v<T, resource::sampler>)
                vkDestroySampler(device.handle(), resource_ptr->handle(), nullptr);

            else if constexpr (std::is_same_v<T, resource::framebuffer>)
                vkDestroyFramebuffer(device.handle(), resource_ptr->handle(), nullptr);

            else if constexpr (std::is_same_v<T, resource::semaphore>)
                vkDestroySemaphore(device.handle(), resource_ptr->handle(), nullptr);

            else if constexpr (std::is_same_v<T, resource::fence>)
                vkDestroyFence(device.handle(), resource_ptr->handle(), nullptr);

            delete resource_ptr;
        }
    };
}

namespace resource
{
    resource_manager::resource_manager(vulkan::device const &device, render::config const &config, resource::memory_manager &memory_manager)
        : device_{device}, config_{config}, memory_manager_{memory_manager},
          resource_deleter_{std::make_shared<resource::resource_manager::resource_deleter>(device, *this)},
          staging_buffer_pool_{std::make_shared<resource::resource_manager::staging_buffer_pool>(device_, *this)}
    {
    }

    std::shared_ptr<resource::buffer>
    resource_manager::create_buffer(std::size_t size_bytes, graphics::BUFFER_USAGE usage, graphics::MEMORY_PROPERTY_TYPE memory_property_types,
                                    graphics::RESOURCE_SHARING_MODE sharing_mode) const
    {
        VkBufferCreateInfo const create_info{
            VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            nullptr,
            0, //VK_BUFFER_CREATE_SPARSE_BINDING_BIT,
            size_bytes,
            convert_to::vulkan(usage),
            convert_to::vulkan(sharing_mode),
            0, nullptr
        };

        VkBuffer handle;

        if (auto result = vkCreateBuffer(device_.handle(), &create_info, nullptr, &handle); result != VK_SUCCESS)
            throw resource::instantiation_fail(fmt::format("failed to create a buffer: {0:#x}", result));

        auto const memory = memory_manager_.allocate_memory(resource::buffer{
            handle, nullptr, size_bytes, usage, sharing_mode
        }, memory_property_types);

        if (memory == nullptr)
            throw memory::bad_allocation("failed to allocate buffer memory"s);

        if (auto result = vkBindBufferMemory(device_.handle(), handle, memory->handle(), memory->offset()); result != VK_SUCCESS)
            throw resource::memory_bind(fmt::format("failed to bind buffer memory: {0:#x}", result));

        std::shared_ptr<resource::buffer> buffer;
        buffer.reset(new resource::buffer{
            handle, memory, size_bytes, usage, sharing_mode
        }, *resource_deleter_);

        return buffer;
    }

    std::shared_ptr<resource::staging_buffer>
    resource_manager::create_staging_buffer(std::size_t size_bytes) const
    {
        auto [offset_bytes, mapped_range] = staging_buffer_pool_->allocate_mapped_range(size_bytes);

        std::shared_ptr<resource::staging_buffer> buffer;

        buffer.reset(new resource::staging_buffer{
            staging_buffer_pool_->buffer(), mapped_range, offset_bytes
        }, *resource_deleter_);

        return buffer;

    #if 0
        auto constexpr buffer_usage_flags = graphics::BUFFER_USAGE::TRANSFER_SOURCE;
        auto constexpr sharing_mode = graphics::RESOURCE_SHARING_MODE::EXCLUSIVE;
        auto constexpr memory_property_types = graphics::MEMORY_PROPERTY_TYPE::HOST_VISIBLE | graphics::MEMORY_PROPERTY_TYPE::HOST_COHERENT;

        VkBufferCreateInfo const create_info{
            VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            nullptr,
            0, //VK_BUFFER_CREATE_SPARSE_BINDING_BIT,
    #ifndef _MSC_VER
        #pragma GCC diagnostic push
        #pragma GCC diagnostic ignored "-Wuseless-cast"
    #endif
            static_cast<VkDeviceSize>(size_bytes),
    #ifndef _MSC_VER
        #pragma GCC diagnostic pop
    #endif
            convert_to::vulkan(buffer_usage_flags),
            convert_to::vulkan(sharing_mode),
            0, nullptr
        };

        VkBuffer handle;

        if (auto result = vkCreateBuffer(device_.handle(), &create_info, nullptr, &handle); result != VK_SUCCESS)
            throw resource::instantiation_fail(fmt::format("failed to create a buffer: {0:#x}", result));

        auto memory = memory_manager_.allocate_memory(resource::buffer{
            handle, nullptr, size_bytes, buffer_usage_flags, sharing_mode
        }, memory_property_types);

        if (memory == nullptr)
            throw memory::bad_allocation("failed to allocate buffer memory"s);

        if (auto result = vkBindBufferMemory(device_.handle(), handle, memory->handle(), memory->offset()); result != VK_SUCCESS)
            throw resource::memory_bind(fmt::format("failed to bind buffer memory: {0:#x}", result));

        std::shared_ptr<resource::staging_buffer> buffer;

        void *mapped_ptr;

        if (auto result = vkMapMemory(device_.handle(), memory->handle(), memory->offset(), size_bytes, 0, &mapped_ptr); result == VK_SUCCESS) {
            buffer.reset(new resource::staging_buffer{
                handle, memory, std::span<std::byte>{reinterpret_cast<std::byte *>(mapped_ptr), size_bytes}, buffer_usage_flags, sharing_mode
            }, *resource_deleter_);
        }

        else throw resource::exception(fmt::format("failed to map staging buffer memory: {0:#x}", result));

        return buffer;
    #endif
    }

    std::shared_ptr<resource::image>
    resource_manager::create_image(graphics::IMAGE_TYPE type, graphics::FORMAT format, render::extent extent, std::uint32_t mip_levels, std::uint32_t samples_count,
                                   graphics::IMAGE_TILING tiling, graphics::IMAGE_USAGE usage_flags, graphics::MEMORY_PROPERTY_TYPE memory_property_types) const
    {
        VkImageCreateInfo const create_info{
            VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
            nullptr,
            0, //VK_IMAGE_CREATE_SPARSE_BINDING_BIT,
            convert_to::vulkan(type),
            convert_to::vulkan(format),
            { extent.width, extent.height, 1 },
            mip_levels,
            1,
            convert_to::vulkan(samples_count),
            convert_to::vulkan(tiling),
            convert_to::vulkan(usage_flags),
            VK_SHARING_MODE_EXCLUSIVE,
            0, nullptr,
            VK_IMAGE_LAYOUT_UNDEFINED
        };

        VkImage handle;

        if (auto result = vkCreateImage(device_.handle(), &create_info, nullptr, &handle); result != VK_SUCCESS)
            throw resource::instantiation_fail(fmt::format("failed to create an image: {0:#x}", result));

        auto const memory = memory_manager_.allocate_memory(resource::image{nullptr, handle, format, tiling, mip_levels, extent}, memory_property_types);

        if (memory == nullptr)
            throw memory::exception("failed to allocate image memory"s);

        if (auto result = vkBindImageMemory(device_.handle(), handle, memory->handle(), memory->offset()); result != VK_SUCCESS)
            throw resource::memory_bind(fmt::format("failed to bind image buffer memory: {0:#x}", result));

        std::shared_ptr<resource::image> image;
        image.reset(new resource::image{memory, handle, format, tiling, mip_levels, extent}, *resource_deleter_);

        return image;
    }

    std::shared_ptr<resource::image_view>
    resource_manager::create_image_view(std::shared_ptr<resource::image> image, graphics::IMAGE_VIEW_TYPE view_type, graphics::IMAGE_ASPECT image_aspect)
    {
        std::shared_ptr<resource::image_view> image_view;

        VkImageViewCreateInfo const create_info{
            VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            nullptr, 0,
            image->handle(),
            convert_to::vulkan(view_type),
            convert_to::vulkan(image->format()),
            { VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY },
            { convert_to::vulkan(image_aspect), 0, image->mip_levels(), 0, 1 }
        };

        VkImageView handle;

        if (auto result = vkCreateImageView(device_.handle(), &create_info, nullptr, &handle); result != VK_SUCCESS)
            throw resource::instantiation_fail(fmt::format("failed to create an image view: {0:#x}", result));

        else image_view.reset(new resource::image_view{handle, image, view_type}, [this] (resource::image_view *ptr_image_view)
        {
            vkDestroyImageView(device_.handle(), ptr_image_view->handle(), nullptr);

            delete ptr_image_view;
        });

        return image_view;
    }

    std::shared_ptr<resource::sampler>
    resource_manager::create_image_sampler(graphics::TEXTURE_FILTER min_filter, graphics::TEXTURE_FILTER mag_filter, graphics::TEXTURE_MIPMAP_MODE mipmap_mode,
                                           float min_lod, float max_lod)
    {
        std::shared_ptr<resource::sampler> sampler;

        VkSamplerCreateInfo const create_info{
            VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
            nullptr, 0,
            convert_to::vulkan(min_filter), convert_to::vulkan(mag_filter),
            convert_to::vulkan(mipmap_mode),
            VK_SAMPLER_ADDRESS_MODE_REPEAT,
            VK_SAMPLER_ADDRESS_MODE_REPEAT,
            VK_SAMPLER_ADDRESS_MODE_REPEAT,
            0.f,
            static_cast<VkBool32>(config_.anisotropy_enabled), config_.max_anisotropy_level,
            VK_FALSE, VK_COMPARE_OP_ALWAYS,
            min_lod, max_lod,
            VK_BORDER_COLOR_INT_OPAQUE_BLACK,
            VK_FALSE
        };

        VkSampler handle;

        if (auto result = vkCreateSampler(device_.handle(), &create_info, nullptr, &handle); result != VK_SUCCESS)
            throw resource::instantiation_fail(fmt::format("failed to create a sampler: {0:#x}", result));

        else sampler.reset(new resource::sampler{handle}, [this] (resource::sampler *ptr_sampler)
        {
            vkDestroySampler(device_.handle(), ptr_sampler->handle(), nullptr);

            delete ptr_sampler;
        }
        );

        return sampler;
    }

    std::shared_ptr<resource::framebuffer>
    resource_manager::create_framebuffer(std::shared_ptr<graphics::render_pass> render_pass, render::extent extent,
                                         std::vector<std::shared_ptr<resource::image_view>> const &attachments)
    {
        std::vector<VkImageView> views;

        std::ranges::transform(attachments, std::back_inserter(views), [] (auto &&attachment)
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

        VkFramebuffer handle = VK_NULL_HANDLE;

        if (auto result = vkCreateFramebuffer(device_.handle(), &create_info, nullptr, &handle); result != VK_SUCCESS)
            throw resource::instantiation_fail(fmt::format("failed to create a framebuffer: {0:#x}", result));

        framebuffer.reset(new resource::framebuffer{handle}, [this] (resource::framebuffer *ptr_framebuffer)
        {
            vkDestroyFramebuffer(device_.handle(), ptr_framebuffer->handle(), nullptr);

            delete ptr_framebuffer;
        });

        return framebuffer;
    }

    std::shared_ptr<resource::semaphore> resource_manager::create_semaphore()
    {
        VkSemaphoreCreateInfo constexpr create_info{
            VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
            nullptr, 0
        };

        std::shared_ptr<resource::semaphore> semaphore;

        VkSemaphore handle;

        if (auto result = vkCreateSemaphore(device_.handle(), &create_info, nullptr, &handle); result == VK_SUCCESS) {
            semaphore.reset(new resource::semaphore{handle}, [this] (resource::semaphore *ptr_semaphore)
            {
                vkDestroySemaphore(device_.handle(), ptr_semaphore->handle(), nullptr);

                delete ptr_semaphore;
            });
        }

        else throw resource::instantiation_fail(fmt::format("failed to create a semaphore: {0:#x}", result));

        return semaphore;
    }

    std::shared_ptr<resource::fence> resource_manager::create_fence(bool create_signaled)
    {
        VkFenceCreateInfo const create_info{
            VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
            nullptr,
            create_signaled ? VK_FENCE_CREATE_SIGNALED_BIT : static_cast<VkFenceCreateFlags>(0)
        };

        std::shared_ptr<resource::fence> fence;

        VkFence handle;

        if (auto result = vkCreateFence(device_.handle(), &create_info, nullptr, &handle); result == VK_SUCCESS) {
            fence.reset(new resource::fence{handle}, [this] (resource::fence *ptr_fence)
            {
                vkDestroyFence(device_.handle(), ptr_fence->handle(), nullptr);

                delete ptr_fence;
            });
        }

        else throw resource::instantiation_fail(fmt::format("failed to create a fence: {0:#x}", result));

        return fence;
    }

    std::shared_ptr<resource::vertex_buffer>
    resource_manager::stage_vertex_data(graphics::vertex_layout const &layout, std::shared_ptr<resource::staging_buffer> staging_buffer, VkCommandPool command_pool)
    {
        auto const container = staging_buffer->mapped_range();

        auto const staging_data_size_bytes = container.size_bytes();
        auto const staging_data_offset_bytes = staging_buffer->offset_bytes();

        if (staging_data_size_bytes > kVERTEX_BUFFER_FIXED_SIZE)
            throw resource::not_enough_memory("staging data size is bigger than vertex buffer max size"s);

        for (auto &&attribute : layout.attributes)
            if (!device_.is_format_supported_as_buffer_feature(attribute.format, graphics::FORMAT_FEATURE::VERTEX_BUFFER))
                throw resource::exception(fmt::format("unsupported vertex attribute format: {0:#x}", attribute.format));

        if (!vertex_buffers_.contains(layout)) {
            auto constexpr usage_flags = graphics::BUFFER_USAGE::TRANSFER_DESTINATION | graphics::BUFFER_USAGE::VERTEX_BUFFER;
            auto constexpr property_flags = graphics::MEMORY_PROPERTY_TYPE::DEVICE_LOCAL;
            auto constexpr sharing_mode = graphics::RESOURCE_SHARING_MODE::EXCLUSIVE;

            auto buffer = create_buffer(kVERTEX_BUFFER_FIXED_SIZE, usage_flags, property_flags, sharing_mode);

            if (buffer == nullptr)
                throw resource::instantiation_fail("failed to create device vertex buffer"s);

            auto vertex_buffer = std::make_shared<resource::vertex_buffer>(buffer, 0u, kVERTEX_BUFFER_FIXED_SIZE, layout);

            vertex_buffers_.emplace(layout, resource::resource_manager::vertex_buffer_set{vertex_buffer});
        }

        auto &&vertex_buffer_set = vertex_buffers_.at(layout);

        auto it = vertex_buffer_set.lower_bound(staging_data_size_bytes);

        if (it == std::end(vertex_buffer_set)) {
            // Allocate next buffer for this particular vertex layout.
            throw resource::not_enough_memory("unsupported case"s);
        }

        if (auto node_handle = vertex_buffer_set.extract(it); node_handle) {
            auto &&vertex_buffer = node_handle.value();
            auto &&device_buffer = vertex_buffer->device_buffer();

            auto copy_regions = std::array{
                VkBufferCopy{ staging_data_offset_bytes, vertex_buffer->offset_bytes(), staging_data_size_bytes }
            };

            copy_buffer_to_buffer(device_, device_.transfer_queue, staging_buffer->handle(), device_buffer->handle(), std::move(copy_regions), command_pool);

            auto offset_bytes = vertex_buffer->offset_bytes() + staging_data_size_bytes;

            it = vertex_buffer_set.insert(
                std::make_shared<resource::vertex_buffer>(
                    vertex_buffer->device_buffer(), offset_bytes, kVERTEX_BUFFER_FIXED_SIZE - offset_bytes, layout
                )
            );

            if (it == std::end(vertex_buffer_set))
                throw resource::instantiation_fail("failed to emplace new vertex buffer set node"s);

            return *it;
        }

        throw resource::instantiation_fail("failed to extract vertex buffer set node"s);
    }

    std::shared_ptr<resource::index_buffer>
    resource_manager::stage_index_data(graphics::INDEX_TYPE index_type, std::shared_ptr<resource::staging_buffer> staging_buffer, VkCommandPool command_pool)
    {
        if (std::ranges::none_of(kSUPPORTED_INDEX_FORMATS, [index_type] (auto type) { return type == index_type; }))
            throw resource::exception(fmt::format("unsupported index type: {0:#x}", index_type));

        auto const container = staging_buffer->mapped_range();

        auto const staging_data_size_bytes = container.size_bytes();
        auto const staging_data_offset_bytes = staging_buffer->offset_bytes();

        if (staging_data_size_bytes > kINDEX_BUFFER_FIXED_SIZE)
            throw resource::not_enough_memory("staging data size is bigger than index buffer max size"s);

        if (!index_buffers_.contains(index_type)) {
            auto constexpr usage_flags = graphics::BUFFER_USAGE::TRANSFER_DESTINATION | graphics::BUFFER_USAGE::INDEX_BUFFER;
            auto constexpr property_flags = graphics::MEMORY_PROPERTY_TYPE::DEVICE_LOCAL;
            auto constexpr sharing_mode = graphics::RESOURCE_SHARING_MODE::EXCLUSIVE;

            auto buffer = create_buffer(kINDEX_BUFFER_FIXED_SIZE, usage_flags, property_flags, sharing_mode);

            if (buffer == nullptr)
                throw resource::instantiation_fail("failed to create device index buffer"s);

            auto index_buffer = std::make_shared<resource::index_buffer>(buffer, 0u, kINDEX_BUFFER_FIXED_SIZE, index_type);

            index_buffers_.emplace(index_type, resource::resource_manager::index_buffer_set{index_buffer});
        }

        auto &&index_buffer_set = index_buffers_.at(index_type);

        auto it = index_buffer_set.lower_bound(staging_data_size_bytes);

        if (it == std::end(index_buffer_set)) {
            // Allocate next buffer for this particular index type.
            throw resource::not_enough_memory("unsupported case"s);
        }

        if (auto node_handle = index_buffer_set.extract(it); node_handle) {
            auto &&index_buffer = node_handle.value();
            auto &&device_buffer = index_buffer->device_buffer();

            auto copy_regions = std::array{
                VkBufferCopy{ staging_data_offset_bytes, index_buffer->offset_bytes(), staging_data_size_bytes }
            };

            copy_buffer_to_buffer(device_, device_.transfer_queue, staging_buffer->handle(), device_buffer->handle(), std::move(copy_regions), command_pool);

            auto offset_bytes = index_buffer->offset_bytes() + staging_data_size_bytes;

            it = index_buffer_set.insert(
                std::make_shared<resource::index_buffer>(
                    index_buffer->device_buffer(), offset_bytes, kINDEX_BUFFER_FIXED_SIZE - offset_bytes, index_type
                )
            );

            if (it == std::end(index_buffer_set))
                throw resource::instantiation_fail("failed to emplace new vertex buffer set node"s);

            else return *it;
        }

        else throw resource::instantiation_fail("failed to extract vertex buffer set node"s);
    }

    std::shared_ptr<resource::image>
    resource_manager::stage_image_data(graphics::IMAGE_TYPE type, graphics::FORMAT format, render::extent extent, graphics::IMAGE_TILING tiling, std::uint32_t mip_levels, std::uint32_t samples_count,
                                       std::shared_ptr<resource::staging_buffer> staging_buffer, [[maybe_unused]] VkCommandPool command_pool)
    {
        if (std::ranges::none_of(kSUPPORTED_IMAGE_FORMATS, [format] (auto t) { return t == format; }))
            throw resource::exception(fmt::format("unsupported image type: {0:#x}", format));

        auto const container = staging_buffer->mapped_range();

        auto const staging_data_size_bytes = container.size_bytes();
//        auto const staging_data_offset_bytes = staging_buffer->offset_bytes();

        if (staging_data_size_bytes > kIMAGE_BUFFER_FIXED_SIZE)
            throw resource::not_enough_memory("staging data size is bigger than image buffer max size"s);

        auto constexpr usage_flags = graphics::IMAGE_USAGE::TRANSFER_SOURCE | graphics::IMAGE_USAGE::TRANSFER_DESTINATION | graphics::IMAGE_USAGE::SAMPLED;
        auto constexpr property_flags = graphics::MEMORY_PROPERTY_TYPE::DEVICE_LOCAL;
//        auto constexpr sharing_mode = graphics::RESOURCE_SHARING_MODE::EXCLUSIVE;

        auto image = create_image(type, format, extent, mip_levels, samples_count, tiling, usage_flags, property_flags);

        if (image == nullptr) {

        }

        return nullptr;
    }
}

namespace resource
{
    template<class T>
    bool resource_manager::buffer_set_comparator<T>::operator() (std::shared_ptr<T> const &lhs, std::shared_ptr<T> const &rhs) const
    {
        return lhs->available_size() < rhs->available_size();
    }

    template<class T>
    template<class S> requires std::is_unsigned_v<S>
    bool resource_manager::buffer_set_comparator<T>::operator() (std::shared_ptr<T> const &buffer, S size_bytes) const
    {
        return buffer->available_size() < size_bytes;
    }

    template<class T>
    template<class S> requires std::is_unsigned_v<S>
    bool resource_manager::buffer_set_comparator<T>::operator() (S size_bytes, std::shared_ptr<T> const &buffer) const
    {
        return buffer->available_size() < size_bytes;
    }
}

std::shared_ptr<resource::buffer>
create_uniform_buffer(resource::resource_manager &resource_manager, std::size_t size)
{
    auto constexpr usage_flags = graphics::BUFFER_USAGE::UNIFORM_BUFFER;
    auto constexpr property_flags = graphics::MEMORY_PROPERTY_TYPE::HOST_VISIBLE | graphics::MEMORY_PROPERTY_TYPE::HOST_COHERENT;
    auto constexpr sharing_mode = graphics::RESOURCE_SHARING_MODE::EXCLUSIVE;

    return resource_manager.create_buffer(size, usage_flags, property_flags, sharing_mode);
}

std::shared_ptr<resource::buffer>
create_coherent_storage_buffer(resource::resource_manager &resource_manager, std::size_t size)
{
    auto constexpr usageFlags = graphics::BUFFER_USAGE::STORAGE_BUFFER;
    auto constexpr propertyFlags = graphics::MEMORY_PROPERTY_TYPE::HOST_VISIBLE | graphics::MEMORY_PROPERTY_TYPE::HOST_COHERENT;
    auto constexpr sharing_mode = graphics::RESOURCE_SHARING_MODE::EXCLUSIVE;

    return resource_manager.create_buffer(size, usageFlags, propertyFlags, sharing_mode);
}

std::shared_ptr<resource::buffer>
create_storage_buffer(resource::resource_manager &resource_manager, std::size_t size)
{
    auto constexpr usage_flags = graphics::BUFFER_USAGE::STORAGE_BUFFER;
    auto constexpr property_flags = graphics::MEMORY_PROPERTY_TYPE::HOST_VISIBLE;
    auto constexpr sharing_mode = graphics::RESOURCE_SHARING_MODE::EXCLUSIVE;

    return resource_manager.create_buffer(size, usage_flags, property_flags, sharing_mode);
}
