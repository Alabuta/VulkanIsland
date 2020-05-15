#pragma once

#include <type_traits>
#include <unordered_map>

#ifndef GLFW_INCLUDE_VULKAN
    #define GLFW_INCLUDE_VULKAN
    #include <GLFW/glfw3.h>
#endif

#include "vulkan/debug.hxx"
#include "platform/window.hxx"


namespace vulkan
{
    class instance;
}

namespace renderer
{
    class platform_surface final {
    public:

        VkSurfaceKHR handle() const noexcept { return handle_; }

    private:

        VkSurfaceKHR handle_{VK_NULL_HANDLE};

        friend vulkan::instance;
    };
}

namespace vulkan
{
    class instance final {
    public:

        explicit instance();
        ~instance();

        VkInstance handle() const noexcept { return handle_; }

        renderer::platform_surface get_platform_surface(platform::window &window);

    private:

        VkInstance handle_{VK_NULL_HANDLE};

        VkDebugReportCallbackEXT debug_report_callback_{VK_NULL_HANDLE};
        VkDebugUtilsMessengerEXT debug_messenger_{VK_NULL_HANDLE};

        std::map<GLFWwindow *const, renderer::platform_surface> platform_surfaces_;

        instance(instance const &) = delete;
        instance(instance &&) = delete;
    };
}
