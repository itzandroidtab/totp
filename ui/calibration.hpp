#pragma once

#include <klib/io/rtc.hpp>

#include "screen.hpp"
#include "numeric_popup.hpp"

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
        numeric_popup<FrameBuffer>& popup;

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
            // reset and go back to the previous screen
            current = steps::enabled;

            // go back
            screen_base::buffer.back();
        }

        void change_screen(const steps current) {
            // get what state we are in
            switch (current) {
                case steps::enabled:
                    popup.configure(
                        "Cal enabled", static_cast<bool>(RtcPeriph::port->CCR & (0x1 << 4)), 
                        0, 1, [&](int32_t value){next(value);},
                        [&](){cancel();}
                    );
                    break;
                case steps::direction:
                    popup.configure(
                        "Cal direction", static_cast<bool>(RtcPeriph::port->CALIBRATION & (0x1 << 17)), 
                        0, 1, [&](int32_t value){next(value);},
                        [&](){cancel();}
                    );
                    break;
                case steps::calibration:
                    popup.configure(
                        "Cal value", RtcPeriph::port->CALIBRATION & 0x1ffff, 
                        0, 0x1ffff, [&](int32_t value){next(value);},
                        [&](){cancel();}
                    );
                    break;
            }

            // change to the popup screen
            screen_base::buffer.change(screen_id::numeric_popup);
        }

    public:
        calibration(numeric_popup<FrameBuffer>& popup): 
            current(steps::enabled), popup(popup)
        {}

        virtual void main(const klib::time::us delta, const input::buttons& buttons) override {
            // check for the first entry
            if (current == steps::enabled) {
                // show the first screen
                change_screen(current);
            }
            else {
                // reset and go back to the previous screen
                current = steps::enabled;

                // go back
                screen_base::buffer.back();
            }
        }

        virtual void draw(FrameBuffer& frame_buffer, const klib::vector2u& offset) override {
            // do nothing on a draw call
        }
    };
}