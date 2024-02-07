#pragma once

#include <cstring>
#include <optional>

#include <klib/delay.hpp>
#include <klib/string.hpp>

#include "screen.hpp"

namespace menu {
    template <
        typename FrameBuffer, typename UsbKeyboard, 
        typename UsbMouse
    >
    class mouse: public screen<FrameBuffer> {
    protected:
        using screen_base = screen<FrameBuffer>;

        // amount of time since the last update
        static inline klib::time::us last_update = {};

    public:
        mouse() {}

        virtual void activate(const screen_id id) override {
            // clear the last time we updated
            last_update = {};

            // disconnect the usb keyboard from the host
            // this prevents a blue screen when we suddenly
            // change to a different usb type
            UsbKeyboard::disconnect();

            // wait a bit to give the host time to detect it
            klib::delay<>(klib::time::ms(500));
            
            // initialize the usb mass storage
            UsbMouse::init();
        }

        virtual void deactivate(const screen_id id) override {
            // disconnect the usb mass storage from the host
            // this prevents a blue screen when we suddenly
            // change to a different usb type
            UsbMouse::disconnect();

            // wait a bit to give the host time to detect it
            klib::delay<>(klib::time::ms(500));

            // initialize the usb mass storage
            UsbKeyboard::init();
        }

        virtual void main(const klib::time::us delta, const input::buttons& buttons) override {
            if (buttons.enter == input::state::long_pressed) {
                // go back to the previous screen
                screen_base::buffer.back();
            }

            // update the time since the last update
            last_update = last_update + delta;

            // check if we need to move the mouse
            if (last_update >= static_cast<klib::time::us>(klib::time::s(10))) {
                // move the mouse
                UsbMouse::device::template write<UsbMouse>(0x00, -1, 0);

                // move it back
                UsbMouse::device::template write<UsbMouse>(0x00, 1, 0);

                // reset the last update
                last_update = {};
            }
        }

        virtual void draw(FrameBuffer& frame_buffer, const klib::vector2u& offset) override {
            // clear the background black
            frame_buffer.clear(klib::graphics::black);

            // draw the current token using the large font
            screen<FrameBuffer>::large_text::template draw<FrameBuffer>(
                frame_buffer, 
                "USB jiggler", 
                klib::vector2i{36, 60} - offset.cast<int32_t>(), 
                klib::graphics::white
            );
        }
    };
}