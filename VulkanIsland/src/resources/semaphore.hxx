#pragma once

#include <iostream>
#include <optional>

#include "main.hxx"
#include "device/device.hxx"


namespace resource
{
    class semaphore final {
    public:

        semaphore(VkSemaphore handle) : handle_{ handle } { }

        VkSemaphore handle() const noexcept { return handle_; }

    private:
        VkSemaphore handle_;
    };
}
