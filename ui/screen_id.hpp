#pragma once

namespace menu {
    enum class screen_id: uint8_t {
        splash = 0,
        totp,
        settings,
        time,
        timezone,
        calibration,
        config,
        mouse,
        numeric_popup,
        string_popup,
    };
}