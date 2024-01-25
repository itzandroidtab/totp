#pragma once

#include <span>

#include <klib/crypt/totp.hpp>
#include <klib/crypt/sha1.hpp>
#include <klib/lookuptable.hpp>
#include <klib/io/systick.hpp>

#include <storage.hpp>
#include <math.hpp>

#include "screen.hpp"

namespace menu::detail {
    template <uint32_t Offset, uint32_t StepSize = 2>
    constexpr int sin_scaled(uint32_t i){
        return Offset * math::sin(math::degrees_to_rad(i * StepSize));
    }

    template <uint32_t Offset, uint32_t StepSize = 2>
    constexpr int cos_scaled(uint32_t i){
        return Offset * math::cos(math::degrees_to_rad(i * StepSize));
    }
}

namespace menu {
    template <typename FrameBuffer, typename Storage, typename Rtc, typename Usb>
    class totp: public screen<FrameBuffer> {
    protected:
        using hash = klib::crypt::sha1;
        using screen_base = screen<FrameBuffer>;

        klib::time::s last_epoch = {};
        klib::time::ms last_update = {};

        char delta_buf[32] = {};
        char epoch_buf[12] = {};
        char current_token_buf[7] = {};
        char next_token_buf[7] = {};
        char seconds_left_buf[7] = {};

        constexpr static uint32_t step_size = 2;
        using lookuptable = klib::lookuptable<360 / step_size, int>;

        constexpr static lookuptable lookuptable_sin[] = {
            lookuptable(detail::sin_scaled<20, step_size>),
            lookuptable(detail::sin_scaled<19, step_size>),
            lookuptable(detail::sin_scaled<18, step_size>),
            lookuptable(detail::sin_scaled<17, step_size>),
            lookuptable(detail::sin_scaled<16, step_size>),
            lookuptable(detail::sin_scaled<15, step_size>),
        };

        constexpr static lookuptable lookuptable_cos[] = {
            lookuptable(detail::cos_scaled<20, step_size>),
            lookuptable(detail::cos_scaled<19, step_size>),
            lookuptable(detail::cos_scaled<18, step_size>),
            lookuptable(detail::cos_scaled<17, step_size>),
            lookuptable(detail::cos_scaled<16, step_size>),
            lookuptable(detail::cos_scaled<15, step_size>),
        };

        template <typename T, typename G> 
        void draw_timer(FrameBuffer& frame_buffer, const klib::vector2i position, const klib::time::ms time, const T sin, const G cos) {
            for (uint32_t i = (time.value * (sin.size / 30)) / 1000; i < sin.size; i++) {
                const uint32_t index = (sin.size + ((sin.size / 4) * 3) + i) % sin.size;

                const klib::vector2i p = position + klib::vector2i{
                    cos.get(index), sin.get(index)
                };

                // make sure the position is inside the framebuffer
                if (((p.y < 0) || (p.y >= static_cast<int32_t>(FrameBuffer::height))) || 
                    ((p.x < 0) || (p.x >= static_cast<int32_t>(FrameBuffer::width))))
                {
                    // skip the pixel if it outside of the framebuffer
                    continue;
                }

                // draw the circle
                frame_buffer.set_pixel(p.cast<uint32_t>(), klib::graphics::blue);
            }
        }

        // current entry
        uint32_t current = 0;

    public:
        totp():
            current(0)
        {}

        virtual void main(const klib::time::us delta, const input::buttons& buttons) override {
            bool totp_changed = false;

            // get all entries
            const auto& entries = Storage::get_entries();

            // if we do not have any entries go directly to usb mode
            if (!entries.size()) {
                // change to the settings menu
                screen_base::buffer.change(screen_id::config);

                return;
            }

            // check if we have any button presses
            if (buttons.enter == input::state::pressed) {
                // send the token to the host if the button is
                // pressed and we are connected
                if (Usb::device::template is_configured<Usb>()) {
                    // write our current token to the usb
                    Usb::device::template write<Usb, false>(
                        current_token_buf, klib::string::strlen(current_token_buf)
                    );
                }
            }
            else if (buttons.enter == input::state::long_pressed) {
                // change to the settings menu
                screen_base::buffer.change(screen_id::settings);
            }
            else if (input::is_pressed(buttons.up)) {                
                // up button pressed
                if (!current) {
                    current = (entries.size() - 1);
                }
                else {
                    current--;
                }

                // mark the totp as changed to force a redraw of
                // all the text buffers
                totp_changed = true;
            }
            else if (input::is_pressed(buttons.down)) {
                // down button pressed
                if (current + 1 >= entries.size()) {
                    current = 0;
                }
                else {
                    current++;
                }

                // mark the totp as changed to force a redraw of
                // all the text buffers
                totp_changed = true;
            }

            // check if we should update the hashes
            if (Rtc::get() != last_epoch) {
                // store the new epoch value
                last_epoch = Rtc::get();

                // get the runtime for the timing circle
                last_update = klib::io::systick<>::get_runtime();

                // update the epoch buffer
                klib::string::itoa(last_epoch.value, epoch_buf);

                // update the seconds left in this cycle
                klib::string::itoa(30 - (last_epoch.value % 30), seconds_left_buf);

                // the time has changed. Mark the totp as changed
                // to force a update on the buffers
                totp_changed = true;
            }

            if (totp_changed) {
                // update the tokens
                const auto current_token = klib::crypt::totp<hash, 6>::hash(
                    reinterpret_cast<const uint8_t*>(entries[current].key.data()), 
                    entries[current].key.size(), last_epoch.value, 30, 0
                );

                klib::string::itoa(current_token, current_token_buf);
                klib::string::set_width(current_token_buf, 6, '0');

                const auto next_token = klib::crypt::totp<hash, 6>::hash(
                    reinterpret_cast<const uint8_t*>(entries[current].key.data()), 
                    entries[current].key.size(), last_epoch.value + 30, 30, 0
                );

                klib::string::itoa(next_token, next_token_buf);
                klib::string::set_width(next_token_buf, 6, '0');
            }

            // get the delta as a string
            klib::string::itoa(delta.value, delta_buf);
        }

        virtual void draw(FrameBuffer& frame_buffer, const klib::vector2u& offset) override {
            // get all entries
            const auto& entries = Storage::get_entries();

            if (!entries.size()) {
                return;
            }

            // clear the background
            frame_buffer.clear(klib::graphics::black);

            // draw the entry text with a small font above the token if we have any
            if (klib::string::strlen(entries[current].str)) {
                // draw the current token using the large font
                screen<FrameBuffer>::small_text.template draw<FrameBuffer>(
                    frame_buffer, 
                    "profile", 
                    klib::vector2i{3, 3} - offset.cast<int32_t>(), 
                    klib::graphics::white
                );

                // draw the current token using the large font
                screen<FrameBuffer>::large_text.template draw<FrameBuffer>(
                    frame_buffer, 
                    entries[current].str, 
                    klib::vector2i{6, 9 + screen<FrameBuffer>::small_font.height} - offset.cast<int32_t>(), 
                    klib::graphics::white
                );
            }

            // draw the current token using the large font
            screen<FrameBuffer>::large_text.template draw<FrameBuffer>(
                frame_buffer, 
                current_token_buf, 
                klib::vector2i{60, 60} - offset.cast<int32_t>(), 
                klib::graphics::white
            );

            // draw the next token using the small font
            screen<FrameBuffer>::small_text.template draw<FrameBuffer>(
                frame_buffer, 
                next_token_buf, 
                klib::vector2i{108, 78} - offset.cast<int32_t>(), 
                klib::graphics::white
            );

            // draw the epoch time using the small font
            screen<FrameBuffer>::small_text.template draw<FrameBuffer>(
                frame_buffer, 
                epoch_buf, 
                klib::vector2i{
                    (240 - 1) - static_cast<int32_t>(
                            klib::string::strlen(epoch_buf) * screen<FrameBuffer>::small_font.width
                        ), 
                    (135 - 1 - screen<FrameBuffer>::small_font.height)
                } - offset.cast<int32_t>(), 
                klib::graphics::white
            );

            // draw using the large font
            screen<FrameBuffer>::small_text.template draw<FrameBuffer>(
                frame_buffer,
                delta_buf, 
                klib::vector2i{1, (135 - 1 - screen<FrameBuffer>::small_font.height)} - offset.cast<int32_t>(), 
                klib::graphics::white
            );

            // get all the data for drawing the circle
            const klib::vector2i position = klib::vector2i{
                204, 68
            } - offset.cast<int32_t>();

            const klib::time::ms runtime = (
                klib::io::systick<>::get_runtime() - last_update
            ) + klib::time::s(last_epoch.value % 30);

            // draw all the circles
            for (uint32_t i = 0; i < sizeof(lookuptable_sin) / sizeof(lookuptable_sin[0]); i++) {
                draw_timer(frame_buffer, position, runtime, lookuptable_sin[i], lookuptable_cos[i]);
            }

            // draw the time left in seconds in the circle
            screen<FrameBuffer>::small_text.template draw<FrameBuffer>(
                frame_buffer, seconds_left_buf, 
                klib::vector2i{
                    (204 + 1) - static_cast<int32_t>((klib::string::strlen(seconds_left_buf) * screen<FrameBuffer>::small_font.width) / 2), 
                    (68 + 1) - (screen<FrameBuffer>::small_font.height / 2)
                } - offset.cast<int32_t>(), 
                klib::graphics::white
            );
        }
    };
}