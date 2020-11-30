#include <unordered_map>
#include <vector>
#include <ranges>

#include <fmt/format.h>

#include "utility/exceptions.hxx"
#include "graphics/graphics_api.hxx"
#include "buffer.hxx"
#include "image.hxx"
#include "semaphore.hxx"
#include "framebuffer.hxx"
#include "renderer/command_buffer.hxx"

#include "resource_manager.hxx"


namespace resource
{
    struct resource_deleter final {
        vulkan::device const &device;

        resource_deleter(vulkan::device const &device) noexcept : device{device} { }

        template<class T>
        void operator() (T *resource_ptr) const
        {
            if constexpr (std::is_same_v<T, resource::buffer>) {
                vkDestroyBuffer(device.handle(), resource_ptr->handle(), nullptr);

                resource_ptr->memory().reset();
            }

            else if constexpr (std::is_same_v<T, resource::staging_buffer>) {
                vkUnmapMemory(device.handle(), resource_ptr->memory()->handle());

                vkDestroyBuffer(device.handle(), resource_ptr->handle(), nullptr);

                resource_ptr->memory().reset();
            }

            else if constexpr (std::is_same_v<T, resource::image>) {
                vkDestroyImage(device.handle(), resource_ptr->handle(), nullptr);

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

            delete resource_ptr;
        }
    };
}

namespace resource
{
    resource_manager::resource_manager(vulkan::device const &device, renderer::config const &config, resource::memory_manager &memory_manager)
        : device_{device}, config_{config}, memory_manager_{memory_manager}, resource_deleter_{std::make_shared<resource::resource_deleter>(device)}
    { }

    std::shared_ptr<resource::buffer>
    resource_manager::create_buffer(std::size_t size_bytes, graphics::BUFFER_USAGE usage, graphics::MEMORY_PROPERTY_TYPE memory_property_types)
    {
        auto constexpr sharing_mode = graphics::RESOURCE_SHARING_MODE::EXCLUSIVE;

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
            convert_to::vulkan(usage),
            convert_to::vulkan(sharing_mode),
            0, nullptr
        };

        VkBuffer handle;

        if (auto result = vkCreateBuffer(device_.handle(), &create_info, nullptr, &handle); result != VK_SUCCESS)
            throw resource::instantiation_fail(fmt::format("failed to create a buffer: {0:#x}"s, result));

        auto memory = memory_manager_.allocate_memory(resource::buffer{
            handle, nullptr, size_bytes, usage, sharing_mode
        }, memory_property_types);

        if (memory == nullptr)
            throw memory::bad_allocation("failed to allocate buffer memory"s);

        if (auto result = vkBindBufferMemory(device_.handle(), handle, memory->handle(), memory->offset()); result != VK_SUCCESS)
            throw resource::memory_bind(fmt::format("failed to bind buffer memory: {0:#x}"s, result));

        std::shared_ptr<resource::buffer> buffer;

        buffer.reset(new resource::buffer{
            handle, memory, size_bytes, usage, sharing_mode
        }, *resource_deleter_);

        return buffer;
    }

    std::shared_ptr<resource::staging_buffer>
    resource_manager::create_staging_buffer(std::size_t size_bytes)
    {
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
            throw resource::instantiation_fail(fmt::format("failed to create a buffer: {0:#x}"s, result));

        auto memory = memory_manager_.allocate_memory(resource::buffer{
            handle, nullptr, size_bytes, buffer_usage_flags, sharing_mode
        }, memory_property_types);

        if (memory == nullptr)
            throw memory::bad_allocation("failed to allocate buffer memory"s);

        if (auto result = vkBindBufferMemory(device_.handle(), handle, memory->handle(), memory->offset()); result != VK_SUCCESS)
            throw resource::memory_bind(fmt::format("failed to bind buffer memory: {0:#x}"s, result));

        std::shared_ptr<resource::staging_buffer> buffer;

        void *mapped_ptr;

        if (auto result = vkMapMemory(device_.handle(), memory->handle(), memory->offset(), size_bytes, 0, &mapped_ptr); result == VK_SUCCESS) {
            buffer.reset(new resource::staging_buffer{
                handle, memory, std::span<std::byte>{reinterpret_cast<std::byte *>(mapped_ptr), size_bytes}, buffer_usage_flags, sharing_mode
            }, *resource_deleter_);
        }

        else throw resource::exception(fmt::format("failed to map staging buffer memory: {0:#x}"s, result));

        return buffer;
    }

    std::shared_ptr<resource::image>
    resource_manager::create_image(graphics::IMAGE_TYPE type, graphics::FORMAT format, renderer::extent extent, std::uint32_t mip_levels, std::uint32_t samples_count,
                                   graphics::IMAGE_TILING tiling, graphics::IMAGE_USAGE usage_flags, graphics::MEMORY_PROPERTY_TYPE memory_property_types)
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
            throw resource::instantiation_fail(fmt::format("failed to create an image: {0:#x}"s, result));

        auto memory = memory_manager_.allocate_memory(resource::image{nullptr, handle, format, tiling, mip_levels, extent}, memory_property_types);

        if (memory == nullptr)
            throw memory::exception("failed to allocate image memory"s);

        if (auto result = vkBindImageMemory(device_.handle(), handle, memory->handle(), memory->offset()); result != VK_SUCCESS)
            throw resource::memory_bind(fmt::format("failed to bind image buffer memory: {0:#x}"s, result));

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
            throw resource::instantiation_fail(fmt::format("failed to create an image view: {0:#x}"s, result));

        else image_view.reset(new resource::image_view{handle, image, view_type}, [this] (resource::image_view *ptr_image_view)
        {
            vkDestroyImageView(device_.handle(), ptr_image_view->handle(), nullptr);

            delete ptr_image_view;
        });

        return image_view;
    }

    std::shared_ptr<resource::sampler>
    resource_manager::create_image_sampler(graphics::TEXTURE_FILTER min_filter, graphics::TEXTURE_FILTER mag_filter, graphics::TEXTURE_MIPMAP_MODE mipmap_mode,
                                           float max_anisotropy, float min_lod, float max_lod)
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
            static_cast<VkBool32>(config_.anisotropy_enabled), max_anisotropy,
            VK_FALSE, VK_COMPARE_OP_ALWAYS,
            min_lod, max_lod,
            VK_BORDER_COLOR_INT_OPAQUE_BLACK,
            VK_FALSE
        };

        VkSampler handle;

        if (auto result = vkCreateSampler(device_.handle(), &create_info, nullptr, &handle); result != VK_SUCCESS)
            throw resource::instantiation_fail(fmt::format("failed to create a sampler: {0:#x}"s, result));

        else sampler.reset(new resource::sampler{handle}, [this] (resource::sampler *ptr_sampler)
        {
            vkDestroySampler(device_.handle(), ptr_sampler->handle(), nullptr);

            delete ptr_sampler;
        }
        );

        return sampler;
    }

    std::shared_ptr<resource::framebuffer>
    resource_manager::create_framebuffer(std::shared_ptr<graphics::render_pass> render_pass, renderer::extent extent,
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
            throw resource::instantiation_fail(fmt::format("failed to create a framebuffer: {0:#x}"s, result));

        else framebuffer.reset(new resource::framebuffer{handle}, [this] (resource::framebuffer *ptr_framebuffer)
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
            }
            );
        }

        else throw resource::instantiation_fail(fmt::format("failed to create a semaphore: {0:#x}"s, result));

        return semaphore;
    }

    std::shared_ptr<resource::vertex_buffer>
    resource_manager::stage_vertex_data(graphics::vertex_layout const &layout, std::shared_ptr<resource::staging_buffer> staging_buffer, VkCommandPool command_pool)
    {
        auto const container = staging_buffer->mapped_ptr();

        auto const staging_data_size_bytes = container.size_bytes();

        if (staging_data_size_bytes > kVERTEX_BUFFER_FIXED_SIZE)
            throw resource::not_enough_memory("staging data size is bigger than vertex buffer max size"s);

        for (auto &&attribute : layout.attributes)
            if (!device_.is_format_supported_as_buffer_feature(attribute.format, graphics::FORMAT_FEATURE::VERTEX_BUFFER))
                throw resource::exception(fmt::format("unsupported vertex attribute format: {0:#x}"s, attribute.format));

        if (!vertex_buffers_.contains(layout)) {
            auto constexpr usage_flags = graphics::BUFFER_USAGE::TRANSFER_DESTINATION | graphics::BUFFER_USAGE::VERTEX_BUFFER;
            auto constexpr property_flags = graphics::MEMORY_PROPERTY_TYPE::DEVICE_LOCAL;

            auto buffer = create_buffer(kVERTEX_BUFFER_FIXED_SIZE, usage_flags, property_flags);

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
                VkBufferCopy{ 0u, vertex_buffer->offset_bytes(), staging_data_size_bytes }
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

            else return *it;
        }

        else throw resource::instantiation_fail("failed to extract vertex buffer set node"s);
    }

    std::shared_ptr<resource::index_buffer>
    resource_manager::stage_index_data(graphics::FORMAT format, std::shared_ptr<resource::staging_buffer> staging_buffer, VkCommandPool command_pool)
    {
        if (std::ranges::none_of(kSUPPORTED_INDEX_FORMATS, [format] (auto f) { return f == format; }))
            throw resource::exception(fmt::format("unsupported index format: {0:#x}"s, format));

        auto const container = staging_buffer->mapped_ptr();

        auto const staging_data_size_bytes = container.size_bytes();

        if (staging_data_size_bytes > kINDEX_BUFFER_FIXED_SIZE)
            throw resource::not_enough_memory("staging data size is bigger than index buffer max size"s);

        if (!index_buffers_.contains(format)) {
            auto constexpr usage_flags = graphics::BUFFER_USAGE::TRANSFER_DESTINATION | graphics::BUFFER_USAGE::INDEX_BUFFER;
            auto constexpr property_flags = graphics::MEMORY_PROPERTY_TYPE::DEVICE_LOCAL;

            auto buffer = create_buffer(kINDEX_BUFFER_FIXED_SIZE, usage_flags, property_flags);

            if (buffer == nullptr)
                throw resource::instantiation_fail("failed to create device index buffer"s);

            auto index_buffer = std::make_shared<resource::index_buffer>(buffer, 0u, kINDEX_BUFFER_FIXED_SIZE, format);

            index_buffers_.emplace(format, resource::resource_manager::index_buffer_set{index_buffer});
        }

        auto &&index_buffer_set = index_buffers_.at(format);

        auto it = index_buffer_set.lower_bound(staging_data_size_bytes);

        if (it == std::end(index_buffer_set)) {
            // Allocate next buffer for this particular index format.
            throw resource::not_enough_memory("unsupported case"s);
        }

        if (auto node_handle = index_buffer_set.extract(it); node_handle) {
            auto &&index_buffer = node_handle.value();
            auto &&device_buffer = index_buffer->device_buffer();

            auto copy_regions = std::array{
                VkBufferCopy{ 0u, index_buffer->offset_bytes(), staging_data_size_bytes }
            };

            copy_buffer_to_buffer(device_, device_.transfer_queue, staging_buffer->handle(), device_buffer->handle(), std::move(copy_regions), command_pool);

            auto offset_bytes = index_buffer->offset_bytes() + staging_data_size_bytes;

            it = index_buffer_set.insert(
                std::make_shared<resource::index_buffer>(
                    index_buffer->device_buffer(), offset_bytes, kINDEX_BUFFER_FIXED_SIZE - offset_bytes, format
                )
            );

            if (it == std::end(index_buffer_set))
                throw resource::instantiation_fail("failed to emplace new vertex buffer set node"s);

            else return *it;
        }

        else throw resource::instantiation_fail("failed to extract vertex buffer set node"s);
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
CreateUniformBuffer(resource::resource_manager &resource_manager, std::size_t size)
{
    auto constexpr usageFlags = graphics::BUFFER_USAGE::UNIFORM_BUFFER;
    auto constexpr propertyFlags = graphics::MEMORY_PROPERTY_TYPE::HOST_VISIBLE | graphics::MEMORY_PROPERTY_TYPE::HOST_COHERENT;

    return resource_manager.create_buffer(size, usageFlags, propertyFlags);
}

std::shared_ptr<resource::buffer>
create_coherent_storage_buffer(resource::resource_manager &resource_manager, std::size_t size)
{
    auto constexpr usageFlags = graphics::BUFFER_USAGE::STORAGE_BUFFER;
    auto constexpr propertyFlags = graphics::MEMORY_PROPERTY_TYPE::HOST_VISIBLE | graphics::MEMORY_PROPERTY_TYPE::HOST_COHERENT;

    return resource_manager.create_buffer(size, usageFlags, propertyFlags);
}

std::shared_ptr<resource::buffer>
CreateStorageBuffer(resource::resource_manager &resource_manager, std::size_t size)
{
    auto constexpr usageFlags = graphics::BUFFER_USAGE::STORAGE_BUFFER;
    auto constexpr propertyFlags = graphics::MEMORY_PROPERTY_TYPE::HOST_VISIBLE;

    return resource_manager.create_buffer(size, usageFlags, propertyFlags);
}
