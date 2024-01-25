#pragma once

#include <klib/io/rtc.hpp>

#include "screen.hpp"
#include "numeric_popup.hpp"

namespace menu {
    template <typename FrameBuffer, typename RtcPeriph, typename Rtc>
    class time: public screen<FrameBuffer> {
    protected:
        using screen_base = screen<FrameBuffer>;

        enum class steps: uint8_t {
            year = 0,
            month,
            day,
            hour,
            minute,
            second,

            // should always be at the end
            count
        };

        steps current;

        // rtc data
        struct dateinfo {
            uint16_t year;
            uint8_t month;
            uint8_t day;
            uint8_t hour;
            uint8_t minute;
            uint8_t second;
        };

        dateinfo date;

        // popup to show items
        numeric_popup<FrameBuffer>& popup;


        void next(int32_t value) {
            // store the value
            switch (current) {
                case steps::year:
                    date.year = static_cast<uint16_t>(value);
                    break;
                case steps::month:
                    date.month = static_cast<uint8_t>(value);
                    break;
                case steps::day:
                    date.day = static_cast<uint8_t>(value);
                    break;
                case steps::hour:
                    date.hour = static_cast<uint8_t>(value);
                    break;
                case steps::minute:
                    date.minute = static_cast<uint8_t>(value);
                    break;
                case steps::second:
                    date.second = static_cast<uint8_t>(value);
                    
                    // we are done. Update the RTC time
                    Rtc::set(klib::io::rtc::datetime_to_epoch(
                        date.year, date.month, 
                        date.day, date.hour, 
                        date.minute, date.second
                    ));

                    // go back one screen to the setings menu
                    screen_base::buffer.back();
                    return;
                default:
                    // unknown state exit
                    return;
            }

            // go to the next state
            current = static_cast<steps>(static_cast<uint8_t>(current) + 1);

            // change to the next screen
            change_screen(current);
        }

        void cancel() {
            // reset and go back to the previous screen
            current = steps::year;

            // go back
            screen_base::buffer.back();
        }

        void change_screen(const steps current) {
            // get what state we are in
            switch (current) {
                case steps::year:
                    popup.configure(
                        "Year", RtcPeriph::port->YEAR, 
                        2020, 2099, [&](int32_t value){next(value);},
                        [&](){cancel();}
                    );
                    break;
                case steps::month:
                    popup.configure(
                        "Month", RtcPeriph::port->MONTH, 
                        1, 12, [&](int32_t value){next(value);},
                        [&](){cancel();}
                    );
                    break;
                case steps::day:
                    {
                        // check for a leap year
                        const bool is_leap_year_month = (date.year & 0b11) == 0 && (date.month == 1);

                        popup.configure(
                            "Day", RtcPeriph::port->DOM, 
                            0, klib::io::rtc::month_days[date.month] + is_leap_year_month, 
                            [&](int32_t value){next(value);},
                            [&](){cancel();}
                        );
                    }
                    break;
                case steps::hour:
                    popup.configure(
                        "Hour", RtcPeriph::port->HRS, 
                        0, 23, [&](int32_t value){next(value);},
                        [&](){cancel();}
                    );
                    break;
                case steps::minute:
                    popup.configure(
                        "Minute", RtcPeriph::port->MIN, 
                        0, 59, [&](int32_t value){next(value);},
                        [&](){cancel();}
                    );
                    break;
                case steps::second:
                    popup.configure(
                        "Second", RtcPeriph::port->SEC, 
                        0, 59, [&](int32_t value){next(value);},
                        [&](){cancel();}
                    );
                    break;
            }

            screen_base::buffer.change(screen_id::numeric_popup);
        }

    public:
        time(numeric_popup<FrameBuffer>& popup): 
            current(steps::year), date{}, popup(popup)
        {}

        virtual void main(const klib::time::us delta, const input::buttons& buttons) override {
            // check for the first entry
            if (current == steps::year) {
                // show the first screen
                change_screen(current);
            }
            else {
                // reset and go back to the previous screen
                current = steps::year;

                // go back
                screen_base::buffer.back();
            }
        }

        virtual void draw(FrameBuffer& frame_buffer, const klib::vector2u& offset) override {
            // do nothing on a draw call
        }
    };
}