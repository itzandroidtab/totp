#include "button.hpp"

namespace input {
    buttons get_state(
            const klib::time::us delta, const bool flipped, std::array<klib::time::us, 3>& timing, 
            const std::array<bool, buttons::amount>& raw)
    {
        // timing parameters for button presses
        constexpr static klib::time::us short_press = 20'000;
        constexpr static klib::time::us long_press = 700'000;
        
        // return value
        std::array<state, buttons::amount> ret = {};

        // check if the buttons are still pressed
        for (uint32_t i = 0; i < buttons::amount; i++) {
            // check for a release of the button
            if (!raw[i]) {
                // no button press. Check if we had a button press before
                if ((timing[i] >= short_press) && (timing[i] != pressed_marker)) {
                    // we had a button press before. Set as pressed
                    ret[i] = state::pressed;

                    // set the timing with the pressed marker to mark
                    // it as already pressed
                    timing[i] = pressed_marker;
                }
                else if (timing[i] == pressed_marker) {
                    // button is released as it was marked with the pressed
                    // marker
                    ret[i] = state::released;

                    // clear the timing
                    timing[i] = 0;
                }
                else {
                    // no change
                    ret[i] = state::no_change;
                }
            }
            else {
                // we have a button press. Add the delta time for 
                // the button press
                if (timing[i] < long_press) {
                    timing[i] = timing[i] + delta;
                }

                // Check if we have passed the threshold for a long press
                if ((timing[i] >= long_press) && (timing[i] != pressed_marker)) {
                    ret[i] = state::long_pressed;

                    // set the timing with the pressed marker to mark
                    // it as already pressed
                    timing[i] = pressed_marker;
                }
                else {
                    // user is still pressing set as no change
                    ret[i] = state::no_change;
                }
            }
        }

        // if we are flipped swap the top and buttom buttons
        if (flipped) {
            return {ret[0], ret[1], ret[2]};
        }
        else {
            return {ret[2], ret[1], ret[0]};
        }
    }

    bool is_pressed(const state button) {
        return button == state::long_pressed || button == state::pressed;
    }
}