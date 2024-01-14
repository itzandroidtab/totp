#pragma once

namespace menu {
    enum class screen_id: uint8_t {
        splash = 0,
        totp,
        settings,
        time,
        calibration,
        config,
        numeric_popup,
    };
}