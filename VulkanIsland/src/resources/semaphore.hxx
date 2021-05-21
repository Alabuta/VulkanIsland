#pragma once

#include <iostream>
#include <optional>

#include "vulkan/device.hxx"


namespace resource
{
    class semaphore final {
    public:

        explicit semaphore(VkSemaphore handle) : handle_{ handle } { }

        VkSemaphore handle() const noexcept { return handle_; }

    private:
        VkSemaphore handle_{VK_NULL_HANDLE};
    };
}
