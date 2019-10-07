#pragma once

#include <map>
#include <optional>

#include <string>
using namespace std::string_literals;

#include "utility/mpl.hxx"
#include "graphics/graphics_api.hxx"
#include "renderer/queues.hxx"

namespace vulkan
{
    class queue_factory final {
    public:

        ;

    private:

        std::map<std::uint32_t, std::uint32_t> family_indices_;
    };
}
