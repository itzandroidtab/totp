#pragma once

#include <klib/graphics/bitmap.hpp>

#include "menu.hpp"
#include "screen.hpp"

namespace menu {
    template <typename FrameBuffer>
    class settings: public screen<FrameBuffer> {
    protected:
        using screen_base = screen<FrameBuffer>;

        enum class item: uint8_t {
            config = 0,
            time,
            calibration,

            // should always be at the end
            count
        };

        item selection;

        // amount of labels
        constexpr static auto label_count = static_cast<uint8_t>(item::count);

        // all the text fields in the settings menu
        constexpr static char labels[label_count][16] = {
            "usb mode",
            "set time",
            "set cal",
        };

        // menu that can draw all the labels
        static inline auto options = menu<label_count, sizeof(labels[0]), 240, 135>(labels);

    public:
        settings():
            selection(item::config)
        {}

        virtual void main(const klib::time::us delta, const input::buttons& buttons) override {
            // check what button is pressed
            if (buttons.enter == input::state::pressed) {
                // enter button pressed. Check what we should do
                switch (selection) {
                    case item::time:
                        screen_base::buffer.change(screen_id::time);
                        break;
                    case item::calibration:
                        screen_base::buffer.change(screen_id::calibration);
                        break;
                    case item::config:
                        screen_base::buffer.change(screen_id::config);
                        break;
                }
            }
            else if (buttons.enter == input::state::long_pressed) {
                // go back if we have a long press
                screen_base::buffer.back();
            }
            else if (input::is_pressed(buttons.down)) {
                // the down button is pressed
                selection = static_cast<item>(
                    (static_cast<uint8_t>(selection) + 1
                ) % static_cast<uint8_t>(item::count));
            }
            else if (input::is_pressed(buttons.up)) {
                // the up button is pressed
                if (!static_cast<uint8_t>(selection)) {
                    selection = static_cast<item>(
                        static_cast<uint8_t>(item::count) - 1
                    );
                }
                else {
                    selection = static_cast<item>(
                        (static_cast<uint8_t>(selection) - 1
                    ) % static_cast<uint8_t>(item::count));
                }
            }
        }

        virtual void draw(FrameBuffer& frame_buffer, const klib::vector2u& offset) override {
            // clear the background black
            frame_buffer.clear(klib::graphics::black);

            // array with all the options
            char buffer[label_count][sizeof(labels[0])] = {};

            // update the labels
            options.draw(
                screen_base::large_text, frame_buffer, 
                offset, static_cast<uint8_t>(selection), 
                buffer, klib::graphics::grey, 
                klib::graphics::white, {}
            );
        }
    };
}