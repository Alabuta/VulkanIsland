#pragma once

#include <bitset>
#include <variant>


namespace platform
{
    namespace mouse_data
    {
        struct buttons final {
            std::bitset<16> value;
        };

        struct relative_coords final {
            float x, y;
        };

        struct wheel final {
            float xoffset, yoffset;
        };

        using raw = std::variant<
            buttons, relative_coords, wheel
        >;
    }

    namespace keyboard_data
    {
        using raw = bool;
    }

    using raw = std::variant<mouse_data::raw, keyboard_data::raw>;
}
