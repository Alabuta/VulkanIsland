#pragma once

#include <bitset>

#include <boost/signals2.hpp>

#include "input_data_types.hxx"



template<class T>
class IInputDevice {
public:
    virtual ~IInputDevice() = default;
};


class MouseInput final {
public:

    class IHandler {
    public:

        virtual ~IHandler() = default;

        using buttons_t = std::bitset<8>;

        virtual void onMove(float x, float y) = 0;
        virtual void onWheel(float delta) = 0;
        virtual void onDown(buttons_t buttons) = 0;
        virtual void onUp(buttons_t buttons) = 0;
    };

    void connect(std::shared_ptr<IHandler> slot);

    void update(input::mouse::RawData &data);

private:

    IHandler::buttons_t buttons_{0};

    boost::signals2::signal<void(float, float)> onMove_;
    boost::signals2::signal<void(float)> onWheel_;
    boost::signals2::signal<void(IHandler::buttons_t)> onDown_;
    boost::signals2::signal<void(IHandler::buttons_t)> onUp_;
};
