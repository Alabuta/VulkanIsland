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

        instance();
        ~instance();

        VkInstance handle() const noexcept { return instance_; }

    private:

        VkInstance instance_{nullptr};
        VkDebugReportCallbackEXT debug_report_callback_{nullptr};

        instance(instance const &) = delete;
        instance(instance &&) = delete;
    };
}
