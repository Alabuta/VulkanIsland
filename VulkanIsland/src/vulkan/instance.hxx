#pragma once

#ifndef GLFW_INCLUDE_VULKAN
    #define GLFW_INCLUDE_VULKAN
    #include <GLFW/glfw3.h>
#endif

#include "vulkan/debug.hxx"


namespace vulkan
{
    class instance final {
    public:

        explicit instance();
        ~instance();

        VkInstance handle() const noexcept { return handle_; }

    private:

        VkInstance handle_{VK_NULL_HANDLE};
        VkDebugReportCallbackEXT debug_report_callback_{VK_NULL_HANDLE};

        instance(instance const &) = delete;
        instance(instance &&) = delete;
    };
}
