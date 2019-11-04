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

        device(vulkan::instance &instance, renderer::platform_surface const *const platform_surface); // TODO
        ~device();

        VkDevice handle() const noexcept { return handle_; };
        VkPhysicalDevice physical_handle() const noexcept { return physical_handle_; };

        vulkan::device_limits const &device_limits() const noexcept { return device_limits_; };

        renderer::swapchain_support_details query_swapchain_support_details(renderer::platform_surface const *const platform_surface) const;

        graphics::graphics_queue graphics_queue;
        graphics::compute_queue compute_queue;
        graphics::transfer_queue transfer_queue;
        graphics::graphics_queue presentation_queue;

    private:

        VkDevice handle_{nullptr};
        VkPhysicalDevice physical_handle_{nullptr};

        vulkan::device_limits device_limits_;

        device() = delete;
        device(vulkan::device const &) = delete;
        device(vulkan::device &&) = delete;

        struct helper;
    };
}
