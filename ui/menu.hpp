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
        constexpr static uint32_t max_visible_lines = (Height / line_pixels);

        // check if we have any partial lines
        constexpr static bool has_partial_lines = (max_visible_lines != ((Height + (line_pixels - 1)) / line_pixels));

        // pointer to the array that has all the labels
        const string *const data;

        // flag if the partial edge is at the top or bottom
        bool partial_top_line = false;

        // the previous indexes for the top and bottom
        klib::vector2i previous_range = {0, klib::min(max_visible_lines, Count)};

        /**
         * @brief Draw a rectangle at a specific position
         * 
         * @tparam FrameBuffer 
         * @param fb 
         * @param col 
         * @param start_position 
         * @param end_position 
         */
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

    public:
        menu(const string *const args):
            data(args)
        {}

        template <typename Font, typename FrameBuffer>
        void draw(FrameBuffer& fb, const klib::vector2u& offset, const uint32_t selected, const char** option,
            const klib::graphics::color foreground, const klib::graphics::color background, const bool* hidden)
        {
            // check if the selection is outside of the current range
            if (selected < previous_range.x || selected >= previous_range.y) {
                bool above;

                // move the range
                if (selected < previous_range.x) {
                    // mark we are above the previous top
                    above = true;

                    // set the selected as the minimum
                    previous_range.x = selected;
                }
                else {
                    // mark we are not above the previous top
                    // but below the previous bottom
                    above = false;

                    // move the max past the selected
                    previous_range.y = selected + 1;
                }

                // calculate the new bottom or top
                uint32_t items = 0;

                // calculate the other position
                for (uint32_t i = 0; i < klib::min(max_visible_lines, Count); i++) {
                    // get the index to check
                    const int32_t index = above ? (selected + i) : (selected - i);

                    // check if we are at 0 or at the max
                    if (index < 0 || index >= Count) {
                        break;
                    }

                    // check if the option is hidden
                    if (hidden != nullptr && hidden[index]) {
                        // skip the option if it is hidden
                        continue;
                    }

                    // increment the before
                    items++;
                }

                // check what to set
                if (above) {
                    previous_range.y = previous_range.x + items;
                }
                else {
                    previous_range.x = previous_range.y - items;
                }

                // update if we have a partial top or bottom
                partial_top_line = !above;
            }

            // the start offset
            constexpr static int32_t start_offset = boarder_pixels + (boarder_pixels / 2);

            // get the starting position
            int32_t current_pixel_h = (partial_top_line ? 
                (start_offset - (Height % line_pixels)) : start_offset
            );

            // get the top. If we have a partial line at the top we move the index. Same for
            // the bottom. If we do not have a partial line at the top we have one at the bottom 
            const uint32_t top = previous_range.x - (partial_top_line && has_partial_lines);
            const uint32_t bottom = previous_range.y + ((!partial_top_line) && has_partial_lines);

            // check if the top is still valid
            for (uint32_t index = top; index < bottom && index < Count; index++) {
                // check if the option is hidden
                if (hidden != nullptr && hidden[index]) {
                    // skip the option if it is hidden
                    continue;
                }

                // get the correct foreground and background to highglight a option
                const klib::graphics::color col_foreground = ((index == selected) ? foreground : background);
                const klib::graphics::color col_background = ((index == selected) ? background : foreground);

                // fill the background
                draw_rectangle(fb, col_background, 
                    klib::vector2i{3, current_pixel_h - static_cast<int32_t>(boarder_pixels / 2)} - offset.cast<int32_t>(),
                    klib::vector2i{Width - 3, current_pixel_h + static_cast<int32_t>(visible_pixels - (boarder_pixels / 2))} - offset.cast<int32_t>()
                );

                // draw the text on the screen
                // write the text to the display
                Font::template draw(
                    fb, data[index], 
                    klib::vector2i{
                        line_boarder_pixels, current_pixel_h
                    } - offset.cast<int32_t>(),
                    col_foreground, klib::graphics::transparent
                );

                // check if we need to show a option for the current item
                if (option != nullptr && option[index] != nullptr) {
                    // get the length of the option string
                    const uint32_t len = klib::string::strnlen(option[index], sizeof(string));

                    // check if the string has any length
                    if (len && (len != sizeof(string))) {
                        // write the value to the display
                        Font::template draw(
                            fb, option[index], 
                            klib::vector2i{
                                static_cast<int32_t>(Width - (line_boarder_pixels + (Font::font::width * len))),
                                current_pixel_h
                            } - offset.cast<int32_t>(),
                            col_foreground, klib::graphics::transparent
                        );
                    }
                }

                // move the current pixel
                current_pixel_h += line_pixels;
            }
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