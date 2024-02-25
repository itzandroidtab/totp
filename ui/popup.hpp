#pragma once

#include <functional>

#include <klib/graphics/bitmap.hpp>

#include "screen.hpp"

namespace menu {
    template <typename FrameBuffer>
    class popup: public screen<FrameBuffer> {
    protected:
        using screen_base = screen<FrameBuffer>;

        // callbacks for when we press a button
        std::function<void(bool)> next;
        std::function<void()> cancel;

        // pointer to the string we should show
        const char *str;
        const char *up_str;
        const char *down_str;

        // max length between the up and down strings
        uint32_t max_length;

        // range of the numeric popup
        bool value;

    public:
        popup(): 
            next(nullptr), cancel(nullptr), str(nullptr), 
            up_str(nullptr), down_str(nullptr), 
            max_length(0), value(false)
        {}

        /**
         * @brief configure the popup
         * 
         * @param str 
         * @param continue 
         * @param cancel 
         */
        void configure(const char* str, const bool value, const char* up_str, 
            const char *down_str, std::function<void(bool)> next, 
            std::function<void()> cancel) 
        {
            // update the string 
            this->str = str;

            // update the options
            this->value = value;
            this->up_str = up_str;
            this->down_str = down_str;

            this->max_length = klib::max(
                klib::string::strlen(up_str),
                klib::string::strlen(down_str)
            );

            // update the callbacks
            this->next = next;
            this->cancel = cancel;
        }

        virtual void main(const klib::time::us delta, const input::buttons& buttons) override {
            // check what button is pressed
            if (buttons.enter == input::state::pressed) {
                // before we do anything go back to the previous item
                // in the buffer
                screen_base::buffer.back();

                // the enter button is pressed. Call the next screen
                if (next) {
                    // call the next function with the value we got
                    next(value);
                }
            }
            else if (buttons.enter == input::state::long_pressed) {
                // before we do anything go back to the previous item
                // in the buffer
                screen_base::buffer.back();

                // the enter button is long pressed. Call the cancel screen
                if (cancel) {
                    // call the next function with the value we got
                    cancel();
                }
            }
            else if (input::is_pressed(buttons.down) || input::is_pressed(buttons.up)) {
                // the down button is pressed
                value ^= true;
            }
        }

        virtual void draw(FrameBuffer& frame_buffer, const klib::vector2u& offset) override {
            // clear the background black
            frame_buffer.clear(klib::graphics::black);

            // get the string we should display
            const char *ptr = value ? up_str : down_str;

            // set the height to 40 pixels
            const int32_t h = 40;

            // calculate the width. At least the same as the height
            const int32_t w = klib::max(h, 
                ((max_length * screen_base::large_text::font::width) + 20) / 2
            );

            // draw the rectangle arround the number
            screen_base::draw_rectangle(frame_buffer, klib::graphics::blue, 
                klib::vector2i{(240 / 2) - w, (134 / 2) - h} - offset.cast<int32_t>(), 
                klib::vector2i{(240 / 2) + w, (134 / 2) + h} - offset.cast<int32_t>()
            );

            // add 2 lines on both sides of the rectangle to round it a bit
            screen_base::draw_rectangle(frame_buffer, klib::graphics::blue, 
                klib::vector2i{(240 / 2) - (w - 1), (134 / 2) - (h - 1)} - offset.cast<int32_t>(), 
                klib::vector2i{(240 / 2) + w, (134 / 2) + (h - 1)} - offset.cast<int32_t>()
            );
            screen_base::draw_rectangle(frame_buffer, klib::graphics::blue, 
                klib::vector2i{(240 / 2) - (w - 2), (134 / 2) - (h - 2)} - offset.cast<int32_t>(), 
                klib::vector2i{(240 / 2) + (w - 1), (134 / 2) + (h - 2)} - offset.cast<int32_t>()
            );

            screen_base::draw_rectangle(frame_buffer, klib::graphics::blue, 
                klib::vector2i{(240 / 2) - w, (134 / 2) - (h - 1)} - offset.cast<int32_t>(), 
                klib::vector2i{(240 / 2) + (w + 1), (134 / 2) + (h - 1)} - offset.cast<int32_t>()
            );
            screen_base::draw_rectangle(frame_buffer, klib::graphics::blue, 
                klib::vector2i{(240 / 2) - (w + 1), (134 / 2) - (h - 2)} - offset.cast<int32_t>(), 
                klib::vector2i{(240 / 2) + (w + 2), (134 / 2) + (h - 2)} - offset.cast<int32_t>()
            );

            // draw the bigmaps of the arrows

            // draw the string using the small font
            screen_base::large_text::template draw<FrameBuffer>(
                frame_buffer, 
                str,
                klib::vector2i{
                    (240 / 2) - 
                    static_cast<int32_t>(
                        (klib::string::strlen(str) * screen_base::large_text::font::width) / 2
                    ), 2
                } - offset.cast<int32_t>(), 
                klib::graphics::white
            );

            // draw the string using the small font
            screen_base::large_text::template draw<FrameBuffer>(
                frame_buffer, 
                ptr,
                klib::vector2i{
                    (240 / 2) - 
                    static_cast<int32_t>(
                        (klib::string::strlen(ptr) * screen_base::large_text::font::width) / 2
                    ), (134 / 2) - (screen_base::large_text::font::height / 2)
                } - offset.cast<int32_t>(), 
                klib::graphics::white
            );
        }
    };
}