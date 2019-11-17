#include <unordered_map>
#include <vector>

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
    struct resource_map final {
        std::unordered_map<framebuffer_invariant, std::shared_ptr<framebuffer>, hash<framebuffer_invariant>> framebuffers;
    };

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
        : device_{device}, config_{config}, memory_manager_{memory_manager},
          resource_map_{std::make_shared<resource::resource_map>()}, resource_deleter_{std::make_shared<resource::resource_deleter>(device)} { };

    std::shared_ptr<resource::buffer>
    resource_manager::create_buffer(std::size_t size_in_bytes, graphics::BUFFER_USAGE usage, graphics::MEMORY_PROPERTY_TYPE memory_property_types)
    {
        VkBufferCreateInfo const create_info{
            VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            nullptr, 0,
            static_cast<VkDeviceSize>(size_in_bytes),
            convert_to::vulkan(usage),
            VK_SHARING_MODE_EXCLUSIVE,
            0, nullptr
        };

        VkBuffer handle;

        if (auto result = vkCreateBuffer(device_.handle(), &create_info, nullptr, &handle); result != VK_SUCCESS)
            throw resource::instantiation_fail(fmt::format("failed to create a buffer: {0:#x}"s, result));

        auto memory = memory_manager_.allocate_memory(resource::buffer{handle, nullptr}, memory_property_types);

        if (memory == nullptr)
            throw memory::exception("failed to allocate buffer memory"s);

        if (auto result = vkBindBufferMemory(device_.handle(), handle, memory->handle(), memory->offset()); result != VK_SUCCESS)
            throw resource::memory_bind(fmt::format("failed to bind buffer memory: {0:#x}"s, result));

        std::shared_ptr<resource::buffer> buffer;

        buffer.reset(new resource::buffer{handle, memory}, *resource_deleter_);

        return buffer;
    }

    std::shared_ptr<resource::image>
    resource_manager::create_image(graphics::IMAGE_TYPE type, graphics::FORMAT format, renderer::extent extent, std::uint32_t mip_levels, std::uint32_t samples_count,
                                   graphics::IMAGE_TILING tiling, graphics::IMAGE_USAGE usage_flags, graphics::MEMORY_PROPERTY_TYPE memory_property_types)
    {
        VkImageCreateInfo const create_info{
            VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
            nullptr, 0,
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
            throw resource::instantiation_fail(fmt::format("failed to create a framebuffer: {0:#x}"s, result));

        else framebuffer.reset(new resource::framebuffer{handle}, [this] (resource::framebuffer *ptr_framebuffer)
        {
            vkDestroyFramebuffer(device_.handle(), ptr_framebuffer->handle(), nullptr);

            delete ptr_framebuffer;
        });

        resource_map_->framebuffers.emplace(std::move(invariant), framebuffer);

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

    std::shared_ptr<resource::vertex_buffer> resource_manager::create_vertex_buffer(graphics::vertex_layout const &layout, std::size_t size_in_bytes)
    {
        if (!vertex_buffers_.contains(layout)) {
            std::shared_ptr<resource::buffer> staging_buffer;
            std::shared_ptr<resource::buffer> device_buffer;

            auto const capacity_in_bytes = kVERTEX_BUFFER_INCREASE_VALUE * size_in_bytes;

            {
                auto constexpr usage_flags = graphics::BUFFER_USAGE::TRANSFER_SOURCE;
                auto constexpr property_flags = graphics::MEMORY_PROPERTY_TYPE::HOST_VISIBLE | graphics::MEMORY_PROPERTY_TYPE::HOST_COHERENT;

                staging_buffer = create_buffer(size_in_bytes, usage_flags, property_flags);

                if (staging_buffer == nullptr)
                    throw resource::instantiation_fail("failed to create staging vertex buffer"s);
            }

            {
                auto constexpr usage_flags = graphics::BUFFER_USAGE::TRANSFER_DESTINATION | graphics::BUFFER_USAGE::VERTEX_BUFFER;
                auto constexpr property_flags = graphics::MEMORY_PROPERTY_TYPE::DEVICE_LOCAL;

                device_buffer = create_buffer(size_in_bytes, usage_flags, property_flags);

                if (device_buffer == nullptr)
                    throw resource::instantiation_fail("failed to create device vertex buffer"s);
            }

            vertex_buffers_.emplace(layout, std::make_shared<resource::vertex_buffer>(device_buffer, staging_buffer, capacity_in_bytes, layout));
        }

        auto &vertex_buffer = vertex_buffers_.at(layout);

        if (vertex_buffer->staging_buffer_size_in_bytes_ < size_in_bytes) {
            auto constexpr usage_flags = graphics::BUFFER_USAGE::TRANSFER_SOURCE;
            auto constexpr property_flags = graphics::MEMORY_PROPERTY_TYPE::HOST_VISIBLE | graphics::MEMORY_PROPERTY_TYPE::HOST_COHERENT;

            vertex_buffer->staging_buffer_ = create_buffer(size_in_bytes, usage_flags, property_flags);

            if (vertex_buffer->staging_buffer_ == nullptr)
                throw resource::instantiation_fail("failed to extend device vertex buffer"s);

            vertex_buffer->staging_buffer_size_in_bytes_ = size_in_bytes;
        }

        if (vertex_buffer->available_memory_size() < size_in_bytes) {
            // TODO: sparse memory binding
        #if NOT_YET_IMPLEMENTED
            auto buffer_handle = vertex_buffer->device_buffer_->handle();
            auto memory = memory_manager_.allocate_memory(buffer_handle, graphics::MEMORY_PROPERTY_TYPE::DEVICE_LOCAL);
        #endif
            throw resource::instantiation_fail("not enough device memory for device vertex buffer"s);
        }

        return vertex_buffer;
    }

    void resource_manager::stage_vertex_buffer_data(std::shared_ptr<resource::vertex_buffer> vertex_buffer, std::vector<std::byte> const &container) const
    {
        if (vertex_buffer == nullptr)
            return;

        auto const size_in_bytes = std::size(container);

        // TODO: sparse memory binding
        if (vertex_buffer->available_memory_size() < size_in_bytes)
            throw resource::not_enough_memory("not enough device memory for vertex buffer"s);

        auto &&memory = vertex_buffer->staging_buffer().memory();

        void *ptr;

        if (auto result = vkMapMemory(device_.handle(), memory->handle(), vertex_buffer->staging_buffer_offset(), size_in_bytes, 0, &ptr); result != VK_SUCCESS)
            throw resource::exception(fmt::format("failed to map staging vertex buffer memory: {0:#x}"s, result));

        else {
            std::uninitialized_copy_n(std::begin(container), size_in_bytes, reinterpret_cast<std::byte *>(ptr));

            vkUnmapMemory(device_.handle(), memory->handle());

            vertex_buffer->offset_ += size_in_bytes;
        }
    }

    void resource_manager::transfer_vertex_buffers_data(VkCommandPool command_pool, graphics::transfer_queue const &transfer_queue) const
    {
        for (auto &&[layout, vertex_buffer] : vertex_buffers_) {
            auto &&staging_buffer = vertex_buffer->staging_buffer();
            auto &&device_buffer = vertex_buffer->device_buffer();

            auto copy_regions = std::array{ VkBufferCopy{ 0, 0, staging_buffer.memory()->size() } };

            CopyBufferToBuffer(device_, transfer_queue, staging_buffer.handle(), device_buffer.handle(), std::move(copy_regions), command_pool);
        }
    }
}
