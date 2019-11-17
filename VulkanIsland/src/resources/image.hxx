#pragma once

#include <memory>
#include <cstddef>

#include "vulkan/device.hxx"
#include "graphics/graphics.hxx"
#include "memory.hxx"
#include "resource.hxx"


namespace renderer
{
    class swapchain;
}

namespace resource
{
    class resource_manager;
}

namespace resource
{
    class image final {
    public:

        VkImage handle() const noexcept { return handle_; }

        graphics::FORMAT format() const noexcept { return format_; }
        graphics::IMAGE_TILING tiling() const noexcept { return tiling_; }

        renderer::extent extent() const noexcept { return extent_; }

        std::uint32_t mip_levels() const noexcept { return mip_levels_; }

        std::shared_ptr<DeviceMemory> memory() const noexcept { return memory_; }
        std::shared_ptr<DeviceMemory> &memory() noexcept { return memory_; }

    private:

        std::shared_ptr<DeviceMemory> memory_;
        
        VkImage handle_{VK_NULL_HANDLE};

        graphics::FORMAT format_{graphics::FORMAT::UNDEFINED};
        graphics::IMAGE_TILING tiling_{graphics::IMAGE_TILING::OPTIMAL};

        std::uint32_t mip_levels_{1};

        renderer::extent extent_;

        image() = delete;
        image(image const &) = delete;
        image(image &&) = delete;

        image(std::shared_ptr<DeviceMemory> memory, VkImage handle, graphics::FORMAT format, graphics::IMAGE_TILING tiling,
              std::uint32_t mip_levels, renderer::extent extent) :
            memory_{memory}, handle_{handle}, format_{format}, tiling_{tiling}, mip_levels_{mip_levels}, extent_{extent} { }

        friend ResourceManager;
        friend resource::resource_manager;
        friend renderer::swapchain;
    };
}

namespace resource
{
    class image_view final {
    public:

        VkImageView handle() const noexcept { return handle_; }

        graphics::IMAGE_VIEW_TYPE type() const noexcept { return type_; }

    private:

        VkImageView handle_{VK_NULL_HANDLE};

        std::shared_ptr<resource::image> image_;

        graphics::IMAGE_VIEW_TYPE type_;

        image_view() = delete;
        image_view(image_view const &) = delete;
        image_view(image_view &&) = delete;

        image_view(VkImageView handle, std::shared_ptr<resource::image> image, graphics::IMAGE_VIEW_TYPE type) :
            handle_{handle}, image_{image}, type_{type} { }

        friend ResourceManager;
        friend resource::resource_manager;
        friend renderer::swapchain;
    };
}

namespace resource
{
    class sampler final {
    public:

        VkSampler handle() const noexcept { return handle_; }

    private:

        VkSampler handle_{VK_NULL_HANDLE};

        sampler() = delete;
        sampler(sampler const &) = delete;
        sampler(sampler &&) = delete;

        sampler(VkSampler handle) noexcept : handle_{handle} { }

        friend ResourceManager;
        friend resource::resource_manager;
    };
}


namespace resource
{
    struct texture final {
        std::shared_ptr<resource::image> image;
        std::shared_ptr<resource::image_view> view;
        std::shared_ptr<resource::sampler> sampler;

        texture(std::shared_ptr<resource::image> image, std::shared_ptr<resource::image_view> view, std::shared_ptr<resource::sampler> sampler) noexcept
            : image{image}, view{view}, sampler{sampler} { }
    };
}

[[nodiscard]] std::optional<graphics::FORMAT>
find_supported_image_format(vulkan::device const &device, std::vector<graphics::FORMAT> const &candidates,
                            graphics::IMAGE_TILING tiling, graphics::FORMAT_FEATURE features);