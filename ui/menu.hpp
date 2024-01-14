#ifndef MENU_HPP
#define MENU_HPP

#include <string_view>

#include <klib/graphics/color.hpp>
#include <klib/string.hpp>

namespace menu {
    template <uint32_t Count, uint32_t Size, uint32_t Width, uint32_t Height>
    class menu {
    protected:
        // type for the string data
        using string = char[Size];

        // pixel information
        constexpr static uint32_t boarder_pixels = 4;
        constexpr static uint32_t visible_pixels = 20;

        constexpr static uint32_t line_boarder_pixels = 10;

        // boarder + visible pixels
        constexpr static uint32_t line_pixels = (visible_pixels + boarder_pixels);

        // calculate the amount of items that fit on a single screen
        constexpr static uint32_t max_visible_lines = (Width / line_pixels);

        // pointer to the array that has all the labels
        const string *const data;

        // previous amount of items shown on the display
        uint32_t previous_shown_amount = 0;

        template <typename FrameBuffer>
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

        /**
         * @brief Fill 
         * 
         * @tparam FrameBuffer 
         * @param fb 
         * @param index 
         * @param col 
         */
        template <typename FrameBuffer>
        constexpr static void fill_background(FrameBuffer& fb, const klib::vector2u& offset, 
            const uint8_t index, const klib::graphics::color& col) 
        {
            // draw a rectangle
            return draw_rectangle(fb, col,
                klib::vector2i{3, static_cast<int32_t>(((index * line_pixels) + boarder_pixels))} - offset.cast<int32_t>(),
                klib::vector2i{Width - 3, static_cast<int32_t>(((index * line_pixels) + (line_pixels)))} - offset.cast<int32_t>()
            );
        }

    public:
        menu(const string *const args):
            data(args)
        {}

        template <typename T, typename FrameBuffer, uint32_t BufferSize>
        void draw(T& writer, FrameBuffer& fb, const klib::vector2u& offset, const uint32_t selected, char option[Count][BufferSize],
            const klib::graphics::color foreground, const klib::graphics::color background, const bool hidden[Count] = {})
        {
            uint32_t onscreen_index = 0;
            uint32_t hidden_by_scroll = 0;

            // draw the menu and the option if it is provided
            for (uint32_t i = 0; i < Count && (onscreen_index < max_visible_lines); i++) {
                // check if the item should be skipped as we are out of the bounds 
                // for scrolling
                const bool scroll = int32_t((int32_t(selected) - i) - max_visible_lines) >= 0;

                // mark if we are hiding the value by scrolling
                hidden_by_scroll += scroll;

                // check if the option is hidden
                if ((hidden != nullptr && hidden[i]) || scroll) {
                    // skip the option if it is hidden
                    continue;
                }

                // get the index we are drawing to the screen on
                const uint32_t position_index = onscreen_index;

                // get the local selection based on the amount of lines we have skipped
                const uint32_t local_selection = selected - hidden_by_scroll;

                // get the correct foreground and background to highglight a option
                const klib::graphics::color col_foreground = ((local_selection == position_index) ? foreground : background);
                const klib::graphics::color col_background = ((local_selection == position_index) ? background : foreground);

                // fill the background
                fill_background(fb, offset, position_index, col_background);

                // draw the text on the screen
                // write the text to the display
                writer.draw(
                    fb, data[i], 
                    klib::vector2i{
                        line_boarder_pixels, 
                        static_cast<int32_t>(
                            boarder_pixels + (boarder_pixels / 2) + 
                            (position_index * line_pixels)
                        )
                    } - offset.cast<int32_t>(),
                    col_foreground, klib::graphics::transparent
                );

                // move the onscreen index
                onscreen_index++;

                // // check if we need to show a option for the current item
                // if (option == nullptr || option[i] == nullptr) {
                //     // skip if we dont have one
                //     continue;
                // }

                // // get the length of the option string
                // const uint32_t len = klib::string::strlen(option[i]);

                // // check if the string has any length
                // if (!len) {
                //     continue;
                // }

                // // write the value to the display
                // writer.draw(
                //     fb, option[i], 
                //     klib::vector2i{
                //         static_cast<int32_t>(Width - line_boarder_pixels + (len * 8)), 
                //         static_cast<int32_t>(3 + position_index * line_boarder_pixels)
                //     } - offset.cast<int32_t>(),
                //     col_foreground, klib::graphics::transparent
                // );
            }

            // check if we need to clear parts of the screen
            for (uint32_t i = onscreen_index; i < previous_shown_amount; i++) {
                // clear any previous items from the screen
                fill_background(fb, offset, i, klib::graphics::black);
            }

            // update the amount of items shown on the display
            previous_shown_amount = onscreen_index;
        }

        template <typename FrameBuffer>
        void draw(FrameBuffer& fb, const uint32_t selected,
            const klib::graphics::color foreground, const klib::graphics::color background,
            const bool hidden[Count] = {})
        {
            // draw using the draw with options
            draw<FrameBuffer, 1>(fb, selected, {}, foreground, background, hidden);
        }

    };
}

#endif