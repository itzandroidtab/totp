#pragma once

#include <klib/io/rtc.hpp>

#include "screen.hpp"
#include "numeric_popup.hpp"

namespace menu {
    template <typename FrameBuffer, typename RtcPeriph>
    class timezone: public screen<FrameBuffer> {
    protected:
        using screen_base = screen<FrameBuffer>;

        // popup to show items
        numeric_popup<FrameBuffer>& popup;

        void next(int32_t value) {
            // store the timezone in the rtc registers
            RtcPeriph::port->GPREG4 = (
                (RtcPeriph::port->GPREG4 & (~0x1f)) | 
                static_cast<uint8_t>(value + 12)
            );

            // go back to the menu
            screen_base::buffer.back();
        }

        void cancel() {
            // go back to the menu
            screen_base::buffer.back();
        }

    public:
        timezone(numeric_popup<FrameBuffer>& popup): 
            popup(popup)
        {}

        virtual void main(const klib::time::us delta, const input::buttons& buttons) override {
            // show the first screen
            popup.configure(
                "GMT", (RtcPeriph::port->GPREG4 & 0x1f) - 12, 
                -12, 14, [&](int32_t value){next(value);},
                [&](){cancel();}
            );

            screen_base::buffer.change(screen_id::numeric_popup);
        }

        virtual void draw(FrameBuffer& frame_buffer, const klib::vector2u& offset) override {
            // clear the background black
            frame_buffer.clear(klib::graphics::black);

            // do nothing on a draw call
        }
    };
}