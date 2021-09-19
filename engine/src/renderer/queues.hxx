#pragma once

#include <variant>

#include "utility/mpl.hxx"
#include "vulkan/device.hxx"
#include "graphics/graphics.hxx"


namespace vulkan
{
    class device;
}

namespace graphics
{
    class queue {
    public:

        VkQueue handle() const noexcept { return handle_; }

        std::uint32_t family() const noexcept { return family_; }
        std::uint32_t index() const noexcept { return index_; }

    private:

        VkQueue handle_{VK_NULL_HANDLE};
        std::uint32_t family_{0}, index_{0};

        friend vulkan::device;
    };

    struct graphics_queue final : public graphics::queue { };
    struct compute_queue final : public graphics::queue { };
    struct transfer_queue final : public graphics::queue { };
}
