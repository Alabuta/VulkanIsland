#pragma once

#include <optional>
#include <map>

#include "utility/mpl.hxx"
#include "renderer/queues.hxx"
#include "device.hxx"


namespace vulkan
{
    class queue_factory final {
    public:

        template<class T>
        [[nodiscard]] T find_queue(vulkan::device const &device, VkSurfaceKHR surface);

    private:

        std::map<std::uint32_t, std::uint32_t> family_indices_;
    };
}
