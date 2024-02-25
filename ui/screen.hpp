#pragma once

#include <cstdint>

#include <klib/dynamic_array.hpp>
#include <klib/graphics/string.hpp>
#include <klib/units.hpp>

#include <button.hpp>

#include "screen_id.hpp"

namespace menu {
    /**
     * @brief Screen buffer
     * 
     * @tparam Depth 
     */
    template <uint32_t Depth>
    class screen_buffer {
    protected:
        klib::dynamic_array<screen_id, Depth> screens;

    public:
        /**
         * @brief Construct a new screen buffer
         * 
         */
        screen_buffer(): 
            screens()
        {
            // push the splash screen when we initialize
            screens.push_back(screen_id::splash);
        }

        /**
         * @brief Change to a specific screen
         * 
         * @param id 
         * @param allow_back 
         */
        void change(const screen_id id, const bool allow_back = true) {
            // check if we are allowed to go back to the current screen
            // using back()
            if (!allow_back) {
                // remove the current screen from the queue
                screens.pop_back();
            }

            // push the screen id to the queue
            screens.push_back(id);
        }

        /**
         * @brief Go back one screen in the queue
         * 
         */
        void back() {
            // remove the last screen from the queue
            screens.pop_back();
        }

        /**
         * @brief Get the screen id we should run
         * 
         * @return screen_id 
         */
        screen_id get() {
            return screens.back();
        }
    };

    /**
     * @brief A screen we can draw on the screen
     * 
     */
    template <typename FrameBuffer>
    class screen {
    private:
        // fonts used in the string functions
        using small_font = klib::graphics::ascii_font_8x8;
        using large_font = klib::graphics::ascii_font_16x16;

    protected:
        // Screen queue. Allows up for 8 screens deep
        static inline screen_buffer<8> buffer = {};

        // small text for in the screens
        using small_text = klib::graphics::string<small_font>;

        // large text for in the screens
        using large_text = klib::graphics::string<large_font>;

        /**
         * @brief Draws a rectangle
         * 
         * @param fb 
         * @param col 
         * @param start_position 
         * @param end_position 
         */
        constexpr static void draw_rectangle(FrameBuffer& fb, const klib::graphics::color col, 
            const klib::vector2i start_position, const klib::vector2i end_position) 
        {
            // draw a rectangle
            for (int32_t y = start_position.y; y < end_position.y; y++) {
                for (int32_t x = start_position.x; x < end_position.x; x++) {
                    // make sure the position is inside the framebuffer
                    if ((y < 0 || y >= static_cast<int32_t>(FrameBuffer::height)) || 
                        (x < 0 || x >= static_cast<int32_t>(FrameBuffer::width)))
                    {
                        // skip the pixel if it outside of the framebuffer
                        continue;
                    }

                    fb.set_pixel(klib::vector2i{x, y}.cast<uint32_t>(), col);
                }
            }
        }

    public:
        screen() {}

        /**
         * @brief get the current screen id
         * 
         * @return screen_id 
         */
        static screen_id get() {
            return buffer.get();
        }

        /**
         * @brief Draw the current screen to the screen at offset.
         * 
         * The offset is as we are drawing the screen in parts
         * 
         * @param frame_buffer the framebuffer to draw to
         * @param offset offset the current framebuffer is at
         */
        virtual void draw(FrameBuffer& frame_buffer, const klib::vector2u& offset) {}

        /**
         * @brief Called when switching to the current screen
         * 
         * @param id
         */
        virtual void activate(const screen_id id) {}

        /**
         * @brief Called multiple times per seconds with the delta
         * 
         * @param delta 
         * @param buttons 
         */
        virtual void main(const klib::time::us delta, const input::buttons& buttons) {}

        /**
         * @brief Called when the screen gets deactivated
         * 
         */
        virtual void deactivate(const screen_id id) {}
    };
}