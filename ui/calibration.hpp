#pragma once

#include <klib/io/rtc.hpp>

#include "screen.hpp"
#include "numeric_popup.hpp"
#include "popup.hpp"

namespace menu {
    template <typename FrameBuffer, typename RtcPeriph>
    class calibration: public screen<FrameBuffer> {
    protected:
        using screen_base = screen<FrameBuffer>;

        enum class steps: uint8_t {
            enabled = 0,
            direction,
            calibration,

            // should always be at the end
            count
        };

        steps current;

        // calibration direction
        bool direction;

        // popup we use to show the numbers
        numeric_popup<FrameBuffer>& num_popup;
        popup<FrameBuffer>& str_popup;

        void next(int32_t value) {
            // store the value
            switch (current) {
                case steps::enabled:
                    // check if we need to enable the calibration
                    if (static_cast<bool>(value)) {
                        // continue to the next step
                        break;
                    }

                    // disable the calibration in the RTC
                    RtcPeriph::port->CCR &= ~(0x1 << 4);

                    // reset the states
                    current = steps::enabled;

                    // go back to the previous screen
                    screen_base::buffer.back();
                    return;
                case steps::direction:
                    direction = static_cast<bool>(value);
                    break;
                case steps::calibration:
                    // setup the calibration if we have any
                    RtcPeriph::port->CALIBRATION = (static_cast<uint32_t>(value) & 0x1ffff) | (direction << 17);

                    // enable the rtc calibration
                    RtcPeriph::port->CCR |= (0x1 << 4);

                    // go back one screen to the setings menu
                    screen_base::buffer.back();
                    return;
                default:
                    // unknown state exit
                    return;
            }

            // go to the next state
            current = static_cast<steps>(static_cast<uint8_t>(current) + 1);

            // change to the next screen
            change_screen(current);
        }
 
        void cancel() {
            if (current == steps::enabled) {
                // go back to the menu
                screen_base::buffer.back();

                return;
            }

            // go back one screen
            current = static_cast<steps>(static_cast<uint32_t>(current) - 1);

            // change to the previous screen
            change_screen(current);
        }

        void change_screen(const steps current) {
            // get what state we are in
            switch (current) {
                case steps::enabled:
                    str_popup.configure(
                        "RTC calibration", static_cast<bool>(RtcPeriph::port->CCR & (0x1 << 4)), 
                        "enabled", "disabled", [&](bool value){next(value);},
                        [&](){cancel();}
                    );
                    break;
                case steps::direction:
                    str_popup.configure(
                        "Cal direction", static_cast<bool>(RtcPeriph::port->CALIBRATION & (0x1 << 17)), 
                        "backward", "forward", [&](bool value){next(value);},
                        [&](){cancel();}
                    );
                    break;
                case steps::calibration:
                    num_popup.configure(
                        "Cal value", RtcPeriph::port->CALIBRATION & 0x1ffff, 
                        0, 0x1ffff, [&](int32_t value){next(value);},
                        [&](){cancel();}
                    );
                    break;
            }

            if (current == steps::calibration) {
                // change to the popup screen
                screen_base::buffer.change(screen_id::numeric_popup);
            }
            else {
                // change to the popup screen
                screen_base::buffer.change(screen_id::string_popup);
            }
        }

    public:
        calibration(numeric_popup<FrameBuffer>& num_popup, popup<FrameBuffer>& str_popup): 
            current(steps::enabled), num_popup(num_popup), str_popup(str_popup)
        {}

        virtual void main(const klib::time::us delta, const input::buttons& buttons) override {
            // change to the first step when we are called. We 
            // are only called from the settings menu. The
            // popup callbacks will skip this by changing 
            // directly to the new popup
            current = steps::enabled;

            // show the first screen
            change_screen(current);
        }

        virtual void draw(FrameBuffer& frame_buffer, const klib::vector2u& offset) override {
            // clear the background black
            frame_buffer.clear(klib::graphics::black);

            // do nothing on a draw call
        }
    };
}