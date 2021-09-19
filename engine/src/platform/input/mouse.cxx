#include "utility/helpers.hxx"
#include "mouse.hxx"


namespace
{
    std::bitset<16> constexpr kPRESSED_MASK{
        0x01 | 0x04 | 0x10 | 0x40 | 0x100
    };

    std::bitset<16> constexpr kDEPRESSED_MASK{
        0x02 | 0x08 | 0x20 | 0x80 | 0x200
    };
}

namespace platform
{
    void mouse::connect_handler(std::shared_ptr<handler_interface> slot)
    {
        using boost::placeholders::_1;
        using boost::placeholders::_2;

        on_move_.connect(decltype(on_move_)::slot_type(
            &handler_interface::on_move, slot.get(), _1, _2
        ).track_foreign(slot));

        on_wheel_.connect(decltype(on_wheel_)::slot_type(
            &handler_interface::on_wheel, slot.get(), _1, _2
        ).track_foreign(slot));

        on_down_.connect(decltype(on_down_)::slot_type(
            &handler_interface::on_down, slot.get(), _1
        ).track_foreign(slot));

        on_up_.connect(decltype(on_up_)::slot_type(
            &handler_interface::on_up, slot.get(), _1
        ).track_foreign(slot));
    }

    void mouse::update(platform::mouse_data::raw &data)
    {
        std::visit(overloaded{
            [this] (platform::mouse_data::relative_coords &coords)
            {
                if (coords.x != 0.f || coords.y != 0.f)
                    on_move_(coords.x, coords.y);
            },
            [this] (platform::mouse_data::wheel wheel)
            {
                on_wheel_(wheel.xoffset, wheel.yoffset);
            },
            [this] (platform::mouse_data::buttons &buttons)
            {
                if (buttons.value.any()) {
                    auto const buttons_bit_count = kPRESSED_MASK.count();

                    for (std::size_t i = 0; i < buttons_bit_count; ++i) {
                        auto const pressed = buttons.value[i];
                        auto const depressed = buttons.value[++i];

#pragma warning(push)
#pragma warning(disable : 4805)
                        buttons_[i / 2] = (buttons_[i / 2] | pressed) & ~depressed;
#pragma warning(pop)
                    }

                    if ((buttons.value & kPRESSED_MASK).any())
                        on_down_(buttons_);

                    if ((buttons.value & kDEPRESSED_MASK).any())
                        on_up_(buttons_);
                }
            },
            [] (auto &&) { }
        }, data);
    }
}
