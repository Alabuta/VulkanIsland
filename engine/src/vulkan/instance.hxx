#pragma once

#include <type_traits>
#include <unordered_map>

#include <volk/volk.h>

#include <GLFW/glfw3.h>

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

        [[nodiscard]] VkSurfaceKHR handle() const noexcept { return handle_; }

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

        instance(instance const &) = delete;
        instance(instance &&) = delete;

        instance const &operator=(instance const &) = delete;
        instance const &operator=(instance &&) = delete;

        [[nodiscard]] VkInstance handle() const noexcept { return handle_; }

        renderer::platform_surface get_platform_surface(platform::window &window);

    private:

        VkInstance handle_{VK_NULL_HANDLE};

        VkDebugReportCallbackEXT debug_report_callback_{VK_NULL_HANDLE};
        VkDebugUtilsMessengerEXT debug_messenger_{VK_NULL_HANDLE};

        std::map<GLFWwindow *const, renderer::platform_surface> platform_surfaces_;
    };
}
