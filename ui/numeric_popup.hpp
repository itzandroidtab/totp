#pragma once

#include <functional>

#include <klib/graphics/bitmap.hpp>

#include "screen.hpp"

namespace menu {
    template <typename FrameBuffer>
    class numeric_popup: public screen<FrameBuffer> {
    protected:
        using screen_base = screen<FrameBuffer>;

        // callbacks for when we press a button
        std::function<void(int32_t)> next;
        std::function<void()> cancel;

        // pointer to the string we should show
        const char *str;

        // range of the numeric popup
        klib::vector2i range;
        int32_t value;

    public:
        numeric_popup(): 
            next(nullptr), cancel(nullptr), str(nullptr), 
            range{}, value(0)
        {}

        /**
         * @brief configure the popup
         * 
         * @param str 
         * @param continue 
         * @param cancel 
         */
        void configure(const char* str, const int32_t value, const int32_t min, 
            const int32_t max, std::function<void(int32_t)> next, 
            std::function<void()> cancel) 
        {
            // update the string 
            this->str = str;

            // update the range
            this->value = value;
            this->range = {min, max};

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
            else if (input::is_pressed(buttons.down)) {
                // the down button is pressed
                if (value <= range.x) {
                    // set to max value in range
                    value = range.y;
                }
                else {
                    value--;
                }
            }
            else if (input::is_pressed(buttons.up)) {
                // the up button is pressed
                if (value >= range.y) {
                    // set to max value in range
                    value = range.x;
                }
                else {
                    value++;
                }
            }
        }

        virtual void draw(FrameBuffer& frame_buffer, const klib::vector2u& offset) override {
            // clear the background black
            frame_buffer.clear(klib::graphics::black);

            // draw the number with the big font
            char buffer[32] = {};

            // convert the number to a value
            klib::string::itoa(value, buffer);

            // draw the rectangle arround the number
            screen_base::draw_rectangle(frame_buffer, klib::graphics::blue, 
                klib::vector2i{240 / 3, (134 / 2) - 40} - offset.cast<int32_t>(), 
                klib::vector2i{(240 / 3) * 2, (134 / 2) + 40} - offset.cast<int32_t>()
            );

            // add 2 lines on both sides of the rectangle to round it a bit
            screen_base::draw_rectangle(frame_buffer, klib::graphics::blue, 
                klib::vector2i{240 / 3 - 1, (134 / 2) - 39} - offset.cast<int32_t>(), 
                klib::vector2i{(240 / 3), (134 / 2) + 39} - offset.cast<int32_t>()
            );
            screen_base::draw_rectangle(frame_buffer, klib::graphics::blue, 
                klib::vector2i{240 / 3 - 2, (134 / 2) - 38} - offset.cast<int32_t>(), 
                klib::vector2i{(240 / 3) - 1, (134 / 2) + 38} - offset.cast<int32_t>()
            );

            screen_base::draw_rectangle(frame_buffer, klib::graphics::blue, 
                klib::vector2i{((240 / 3) * 2), (134 / 2) - 39} - offset.cast<int32_t>(), 
                klib::vector2i{((240 / 3) * 2) + 1, (134 / 2) + 39} - offset.cast<int32_t>()
            );
            screen_base::draw_rectangle(frame_buffer, klib::graphics::blue, 
                klib::vector2i{((240 / 3) * 2) + 1, (134 / 2) - 38} - offset.cast<int32_t>(), 
                klib::vector2i{((240 / 3) * 2) + 2, (134 / 2) + 38} - offset.cast<int32_t>()
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
                buffer,
                klib::vector2i{
                    (240 / 2) - 
                    static_cast<int32_t>(
                        (klib::string::strlen(buffer) * screen_base::large_text::font::width) / 2
                    ), (134 / 2) - (screen_base::large_text::font::height / 2)
                } - offset.cast<int32_t>(), 
                klib::graphics::white
            );
        }
    };
}