#pragma once

#include <bitset>
#include <tuple>

#include <boost/signals2.hpp>



using i8 = std::int8_t;
using i16 = std::int16_t;
using i32 = std::int32_t;
using i64 = std::int64_t;

using u8 = std::uint8_t;
using u16 = std::uint16_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;


template<class T>
class IInputDevice {
public:
    virtual ~IInputDevice() = default;
};


struct raw_mouse_t {
    std::bitset<16> buttons{0};
};

class MouseInput final {
public:

    using buttons_t = std::bitset<8>;
    using relative_pos_t = std::pair<std::float_t, std::float_t>;
    using wheel_t = std::float_t;

    using InputData = std::variant<
        buttons_t,
        relative_pos_t,
        wheel_t
    >;

    class IHandler {
    public:

        virtual ~IHandler() = default;

        virtual void onMove(i64 x, i64 y) = 0;
        virtual void onWheel(i16 delta) = 0;
        virtual void onDown(buttons_t buttons) = 0;
        virtual void onUp(buttons_t buttons) = 0;
    };

    void connect(std::shared_ptr<IHandler> slot);

    void update(InputData &data);

private:

    buttons_t buttons_{0};

    boost::signals2::signal<void(i32, i32)> onMove_;
    boost::signals2::signal<void(i32)> onWheel_;
    boost::signals2::signal<void(buttons_t)> onDown_;
    boost::signals2::signal<void(buttons_t)> onUp_;
};
