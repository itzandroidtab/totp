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
    template <typename FrameBuffer, typename Storage, typename Rtc, typename RtcPeriph, typename Usb>
    class totp: public screen<FrameBuffer> {
    protected:
        using hash = klib::crypt::sha1;
        using screen_base = screen<FrameBuffer>;

        klib::time::s last_epoch = {};
        klib::time::ms last_runtime = {};
        klib::time::ms last_epoch_runtime = {};
        uint8_t last_interval = 0;

        char delta_buf[32] = {};
        char epoch_buf[12] = {};
        char current_token_buf[16] = {};
        char next_token_buf[16] = {};
        char seconds_left_buf[7] = {};
        char index_buf[6] = {};

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
        void draw_timer(FrameBuffer& frame_buffer, const uint32_t interval, const klib::vector2i position, const klib::time::ms time, const T sin, const G cos) {
            for (uint32_t i = (time.value * (sin.size / interval)) / 1000; i < sin.size; i++) {
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

        /**
         * @brief Get get the token based on the entry and the current time
         * 
         * @param entry 
         * @param current 
         * @return uint32_t 
         */
        uint32_t get_token(const storage::entry entry, const klib::time::s current) {
            // check the amount of digits we should return
            switch (entry.digits) {
                case storage::digit::digits_6:
                    return klib::crypt::totp<hash, 6>::hash(
                        reinterpret_cast<const uint8_t*>(entry.key.data()), 
                        entry.key.size(), current.value, entry.interval, 0
                    );
                case storage::digit::digits_8:
                    return klib::crypt::totp<hash, 8>::hash(
                        reinterpret_cast<const uint8_t*>(entry.key.data()), 
                        entry.key.size(), current.value, entry.interval, 0
                    );  
            }

            // for all the other formats return 0
            return 0;
        }

    public:
        totp():
            current(((RtcPeriph::port->GPREG4 >> 5) & 0x1f))
        {}

        virtual void activate(const screen_id id) override {
            // check if we need to reset the current index
            if (current >= Storage::get_entries().size()) {
                current = 0;
            }
        }

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

                // update the rtc register
                RtcPeriph::port->GPREG4 = (RtcPeriph::port->GPREG4 & ~(0x1f << 5)) | ((current & 0x1f) << 5);

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

                // update the rtc register
                RtcPeriph::port->GPREG4 = (RtcPeriph::port->GPREG4 & ~(0x1f << 5)) | ((current & 0x1f) << 5);

                // mark the totp as changed to force a redraw of
                // all the text buffers
                totp_changed = true;
            }

            // get the rtc time
            const auto time = Rtc::get();

            // get a reference to the current entry
            const auto& entry = entries[current];

            // check if we should update the hashes
            if ((time != last_epoch) || (last_interval != entry.interval)) {
                // only update the last epoch and runtime when the
                // time has changed
                if (time != last_epoch) {
                    // store the new epoch value
                    last_epoch = time;

                    // get the runtime for the timing circle
                    last_epoch_runtime = klib::io::systick<>::get_runtime();

                    // update the epoch buffer
                    klib::string::itoa(last_epoch.value, epoch_buf);
                }

                // update the seconds left in this cycle
                klib::string::itoa(
                    entry.interval - (last_epoch.value % entry.interval), 
                    seconds_left_buf
                );

                // the time has changed. Mark the totp as changed
                // to force a update on the buffers
                totp_changed = true;

                // update the last interval
                last_interval = entry.interval;
            }

            if (totp_changed) {
                // update the tokens
                const uint32_t current_token = get_token(entry, last_epoch);

                // copy the token to the buffer and set the width
                // based on the amount of digits
                klib::string::itoa(current_token, current_token_buf);
                klib::string::set_width(current_token_buf, 
                    static_cast<uint8_t>(entry.digits), '0'
                );

                // get the next token
                const uint32_t next_token = get_token(
                    entry, last_epoch + klib::time::s(entry.interval)
                );

                // copy the token to the buffer and set the width
                // based on the amount of digits
                klib::string::itoa(next_token, next_token_buf);
                klib::string::set_width(next_token_buf, 
                    static_cast<uint8_t>(entry.digits), '0'
                );

                // get the width we should use for the index string
                const uint32_t index_width = klib::string::detail::count_chars(entries.size());

                // only display it while we have less than 100 entries
                if (entries.size() > 1 && index_width <= 2) {
                    // copy the current index to the buffer
                    klib::string::itoa(current + 1, index_buf);
                    klib::string::set_width(index_buf, index_width, ' ');

                    klib::string::strcat(index_buf, "\\");

                    char *const ptr = index_buf + klib::string::strlen(index_buf);
                    klib::string::itoa(entries.size(), ptr);
                    klib::string::set_width(ptr, index_width, ' ');
                }
            }

            // get the delta as a string
            klib::string::itoa(delta.value, delta_buf);

            // set the last time we updated
            last_runtime = klib::io::systick<>::get_runtime();
        }

        virtual void draw(FrameBuffer& frame_buffer, const klib::vector2u& offset) override {
            // get all entries
            const auto& entries = Storage::get_entries();

            if (!entries.size()) {
                return;
            }

            // get a reference to the current entry
            const auto& entry = entries[current];

            // clear the background
            frame_buffer.clear(klib::graphics::black);

            // draw the entry text with a small font above the token if we have any
            if (klib::string::strlen(entry.str)) {
                // draw the title at the top of the screen
                screen_base::small_text::template draw<FrameBuffer>(
                    frame_buffer, 
                    "profile", 
                    klib::vector2i{
                        (240 - static_cast<int32_t>(klib::string::strlen("profile") * screen_base::small_text::font::width)) / 2, 3
                    } - offset.cast<int32_t>(), 
                    klib::graphics::white
                );

                // draw the profile name below the title
                screen_base::large_text::template draw<FrameBuffer>(
                    frame_buffer, 
                    entry.str, 
                    klib::vector2i{
                        (240 - static_cast<int32_t>(klib::string::strlen(entry.str) * screen_base::large_text::font::width)) / 2, 
                        9 + screen_base::small_text::font::height
                    } - offset.cast<int32_t>(), 
                    klib::graphics::white
                );
            }

            // draw the current token using the large font
            screen_base::large_text::template draw<FrameBuffer>(
                frame_buffer, 
                current_token_buf, 
                klib::vector2i{
                    static_cast<int32_t>(
                        156 - (klib::string::strlen(current_token_buf) * screen_base::large_text::font::width)
                    ), 60
                } - offset.cast<int32_t>(), 
                klib::graphics::white
            );

            // draw the next token using the small font
            screen_base::small_text::template draw<FrameBuffer>(
                frame_buffer, 
                next_token_buf, 
                klib::vector2i{
                    static_cast<int32_t>(
                        156 - (klib::string::strlen(next_token_buf) * screen_base::small_text::font::width)
                    ), 78
                } - offset.cast<int32_t>(),
                klib::graphics::white
            );

            // draw the epoch time using the small font
            screen_base::small_text::template draw<FrameBuffer>(
                frame_buffer, 
                epoch_buf, 
                klib::vector2i{
                    (240 - 1) - static_cast<int32_t>(
                            klib::string::strlen(epoch_buf) * screen_base::small_text::font::width
                        ), 
                    (135 - 1 - screen_base::small_text::font::height)
                } - offset.cast<int32_t>(), 
                klib::graphics::white
            );

            // draw using the large font
            screen_base::small_text::template draw<FrameBuffer>(
                frame_buffer,
                delta_buf, 
                klib::vector2i{1, (135 - 1 - screen_base::small_text::font::height)} - offset.cast<int32_t>(), 
                klib::graphics::white
            );

            // get all the data for drawing the circle
            const klib::vector2i position = klib::vector2i{
                204, 68
            } - offset.cast<int32_t>();

            const klib::time::ms runtime = (
                (last_runtime - last_epoch_runtime) + static_cast<klib::time::ms>(
                    klib::time::s(last_epoch.value % entry.interval)
                )
            );

            // draw all the circles
            for (uint32_t i = 0; i < sizeof(lookuptable_sin) / sizeof(lookuptable_sin[0]); i++) {
                draw_timer(frame_buffer, entry.interval, position, runtime, lookuptable_sin[i], lookuptable_cos[i]);
            }

            // draw the time left in seconds in the circle
            screen_base::small_text::template draw<FrameBuffer>(
                frame_buffer, seconds_left_buf, 
                klib::vector2i{
                    (204 + 1) - static_cast<int32_t>((klib::string::strlen(seconds_left_buf) * screen_base::small_text::font::width) / 2), 
                    (68 + 1) - (screen_base::small_text::font::height / 2)
                } - offset.cast<int32_t>(), 
                klib::graphics::white
            );

            // draw using the large font
            screen_base::small_text::template draw<FrameBuffer>(
                frame_buffer,
                index_buf, 
                klib::vector2i{3, 3} - offset.cast<int32_t>(), 
                klib::graphics::white
            );
        }
    };
}