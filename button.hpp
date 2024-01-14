#pragma once

#include <array>

#include <klib/units.hpp>

namespace input {
    // pressed button marker
    constexpr static klib::time::us pressed_marker = 0xffffffff;

    /**
     * @brief Button states
     * 
     */
    enum state: uint8_t {
        released = 0,
        pressed = 1,
        no_change = 2,
        long_pressed = 3,
    };

    /**
     * @brief Button information for the screens
     * 
     */
    struct buttons {
        // amount of buttons we have
        constexpr static uint32_t amount = 3;

        // state of the up button
        state up;

        // state of the enter button
        state enter;

        // state of the down button
        state down;
    };

    /**
     * @brief Get the button state using the raw input + old timing information
     * 
     * @param delta 
     * @param flipped 
     * @param timing 
     * @param button0 
     * @param button1 
     * @param button2 
     * @return menu::buttons 
     */
    buttons get_state(
        const klib::time::us delta, const bool flipped, std::array<klib::time::us, 3>& timing, 
        const std::array<bool, buttons::amount>& raw
    );
    
    /**
     * @brief Returns if a button is long or short pressed
     * 
     * @return true 
     * @return false 
     */
    bool is_pressed(const state button);
}
