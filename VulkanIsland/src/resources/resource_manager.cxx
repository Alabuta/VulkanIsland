#include <unordered_map>
#include <vector>

#include <fmt/format.h>

#include "buffer.hxx"
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
    resource_manager::resource_manager(vulkan::device const &device, renderer::config const &config, MemoryManager &memory_manager)
        : device_{device}, config_{config}, resource_map_{std::make_shared<resource::resource_map>()}, memory_manager_{memory_manager} { };

    std::shared_ptr<resource::buffer>
    resource_manager::create_buffer(std::size_t size_in_bytes, graphics::BUFFER_USAGE usage, graphics::MEMORY_PROPERTY_TYPE memory_property_type)
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
            throw std::runtime_error(fmt::format("failed to create a buffer: {0:#x}\n"s, result));

        auto constexpr linear_memory = true;

        auto memory = memory_manager_.AllocateMemory(handle, convert_to::vulkan(memory_property_type), linear_memory);

        if (memory == nullptr)
            throw std::runtime_error("failed to allocate buffer memory\n"s);

        std::shared_ptr<resource::buffer> buffer;

        if (auto result = vkBindBufferMemory(device_.handle(), handle, memory->handle(), memory->offset()); result != VK_SUCCESS)
            throw std::runtime_error(fmt::format("failed to bind buffer memory: {0:#x}\n"s, result));

        else buffer.reset(new resource::buffer{memory, handle}, [this] (resource::buffer *ptr_buffer)
        {
            vkDestroyBuffer(device_.handle(), ptr_buffer->handle(), nullptr);
            ptr_buffer->memory().reset();

            delete ptr_buffer;
        });

        return buffer;
    }

    std::shared_ptr<resource::image>
    resource_manager::create_image(graphics::IMAGE_TYPE type, graphics::FORMAT format, renderer::extent extent, std::uint32_t mip_levels, std::uint32_t samples_count,
                                   graphics::IMAGE_TILING tiling, graphics::IMAGE_USAGE usage_flags, graphics::MEMORY_PROPERTY_TYPE memory_property_type)
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
            throw std::runtime_error(fmt::format("failed to create an image: {0:#x}\n"s, result));

        std::shared_ptr<resource::image> image;

        auto const linear_memory = tiling == graphics::IMAGE_TILING::LINEAR;

        auto memory = memory_manager_.AllocateMemory(handle, convert_to::vulkan(memory_property_type), linear_memory);

        if (memory == nullptr)
            throw std::runtime_error("failed to allocate image memory\n"s);

        if (auto result = vkBindImageMemory(device_.handle(), handle, memory->handle(), memory->offset()); result != VK_SUCCESS)
            throw std::runtime_error(fmt::format("failed to bind image buffer memory: {0:#x}\n"s, result));

        else image.reset(new resource::image{memory, handle, format, tiling, mip_levels, extent}, [this] (resource::image *ptr_image)
        {
            vkDestroyImage(device_.handle(), ptr_image->handle(), nullptr);
            ptr_image->memory().reset();

            delete ptr_image;
        });

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
            throw std::runtime_error(fmt::format("failed to create an image view: {0:#x}\n"s, result));

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
            throw std::runtime_error(fmt::format("failed to create a sampler: {0:#x}\n"s, result));

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
            throw std::runtime_error(fmt::format("failed to create a framebuffer: {0:#x}\n"s, result));

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

        else throw std::runtime_error(fmt::format("failed to create a semaphore: {0:#x}\n"s, result));

        return semaphore;
    }
}
