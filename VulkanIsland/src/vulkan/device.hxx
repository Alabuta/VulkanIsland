#pragma once

#include <memory>

#include "utility/mpl.hxx"
#include "instance.hxx"
#include "device_limits.hxx"
#include "renderer/queues.hxx"


namespace renderer
{
    class platform_surface;

    struct surface_format final {
        graphics::FORMAT format;
        graphics::COLOR_SPACE color_space;
    };

    struct swapchain_support_details final {
        VkSurfaceCapabilitiesKHR surface_capabilities;

        std::vector<renderer::surface_format> surface_formats;
        std::vector<graphics::PRESENTATION_MODE> presentation_modes;
    };
}


namespace vulkan
{
    class device final {
    public:

        explicit device(vulkan::instance &instance, renderer::platform_surface platform_surface);
        ~device();

        VkDevice handle() const noexcept { return handle_; };
        VkPhysicalDevice physical_handle() const noexcept { return physical_handle_; };

        vulkan::device_limits const &device_limits() const noexcept { return device_limits_; };

        renderer::swapchain_support_details query_swapchain_support_details(renderer::platform_surface platform_surface) const;

        bool is_format_supported_as_buffer_feature(graphics::FORMAT format, graphics::FORMAT_FEATURE features) const noexcept;

        graphics::graphics_queue graphics_queue;
        graphics::compute_queue compute_queue;
        graphics::transfer_queue transfer_queue;
        graphics::graphics_queue presentation_queue;

    private:

        vulkan::instance &instance_;

        VkDevice handle_{VK_NULL_HANDLE};
        VkPhysicalDevice physical_handle_{VK_NULL_HANDLE};

        vulkan::device_limits device_limits_;

        device() = delete;
        device(vulkan::device const &) = delete;
        device(vulkan::device &&) = delete;

        struct queue_helper;
    };
}
