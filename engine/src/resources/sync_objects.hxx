#pragma once

#include <iostream>
#include <optional>

#include "vulkan/device.hxx"


namespace resource
{
    class semaphore final {
    public:

        explicit semaphore(VkSemaphore handle) : handle_{handle} { }

        VkSemaphore handle() const noexcept { return handle_; }

    private:
        VkSemaphore handle_{VK_NULL_HANDLE};
    };

    class fence final {
    public:

        explicit fence(VkFence handle) : handle_{handle} { }

        VkFence handle() const noexcept { return handle_; }
        VkFence *handle_ptr() noexcept { return &handle_; }

    private:
        VkFence handle_{VK_NULL_HANDLE};
    };
}
