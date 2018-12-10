#include "mouse_input.hxx"

namespace
{
std::bitset<16> constexpr kPRESSED_MASK{
    0x01 | 0x04 | 0x10 | 0x40 | 0x100
};

std::bitset<16> constexpr kDEPRESSED_MASK{
    0x02 | 0x08 | 0x20 | 0x80 | 0x200
};

template<class... Ts>
struct overloaded : Ts... {
    using Ts::operator()...;
};

template<class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;
}

void MouseInput::connect(std::shared_ptr<IHandler> slot)
{
    onMove_.connect(decltype(onMove_)::slot_type(&IHandler::onMove, slot.get(), _1, _2).track_foreign(slot));

    onWheel_.connect(decltype(onWheel_)::slot_type(&IHandler::onWheel, slot.get(), _1).track_foreign(slot));

    onDown_.connect(decltype(onDown_)::slot_type(&IHandler::onDown, slot.get(), _1).track_foreign(slot));

    onUp_.connect(decltype(onUp_)::slot_type(&IHandler::onUp, slot.get(), _1).track_foreign(slot));
}

void MouseInput::update(InputData &data)
{
    /*switch (data.usFlags) {
        case MOUSE_MOVE_RELATIVE:
            if (data.lLastX || data.lLastY)
                onMove_(data.lLastX, data.lLastY);
            break;
    }  */  
    
    // if (data.buttons.any()) {
    //     auto const buttonsBitCount = kPRESSED_MASK.count();

    //     for (std::size_t i = 0; i < buttonsBitCount; ++i) {
    //         auto const pressed = data.buttons[i];
    //         auto const depressed = data.buttons[++i];

    //         buttons_[i / 2] = (buttons_[i / 2] | pressed) & ~depressed;
    //     }

    //     if ((data.buttons & kPRESSED_MASK).any())
    //         onDown_(buttons_);

    //     if ((data.buttons & kDEPRESSED_MASK).any())
    //         onUp_(buttons_);
    // }

    /*switch (data.usButtonFlags) {
        case RI_MOUSE_WHEEL:
            onWheel_(static_cast<i16>(data.usButtonData));
            break;

        default:
            break;
    }*/

    std::visit(overloaded{
        [] (buttons_t &&buttons)
        {
            ;
        },
        [] (auto &&) { }
    }, data);
}
