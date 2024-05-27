#include <klib/klib.hpp>
#include <klib/stream.hpp>

#include "ui/screen.hpp"
#include "ui/totp.hpp"
#include "ui/splash.hpp"
#include "ui/settings.hpp"
#include "ui/numeric_popup.hpp"
#include "ui/popup.hpp"
#include "ui/time.hpp"
#include "ui/calibration.hpp"
#include "ui/config.hpp"
#include "ui/mouse.hpp"

#include "button.hpp"
#include "storage.hpp"

#include <io/ssp.hpp>
#include <io/rtc.hpp>
#include <io/dma.hpp>
#include <io/usb.hpp>
#include <io/system.hpp>
#include <io/flash.hpp>

#include <klib/usb/device/mass_storage.hpp>
#include <klib/usb/device/keyboard.hpp>
#include <klib/usb/device/mouse.hpp>

#include <klib/hardware/display/st7789.hpp>
#include <klib/graphics/framebuffer.hpp>
#include <klib/graphics/framebuffer_modifier.hpp>

#include <klib/filesystem/virtual_fat.hpp>

namespace target = klib::target;

constexpr static klib::time::ms screen_timeout = 60'000;
constexpr static uint32_t fps_frametime = (1'000'000) / 60;

// using for the ssp (spi port)
using ssp = klib::target::io::ssp<klib::target::io::periph::lqfp_80::ssp1<>>;

// using for the display io
using blk = klib::target::io::pin_out<klib::target::pins::package::lqfp_80::p60>;
using rst = klib::target::io::pin_out<klib::target::pins::package::lqfp_80::p68>;
using dc = klib::target::io::pin_out<klib::target::pins::package::lqfp_80::p65>;
using cs = klib::target::io::pin_out<klib::target::pins::package::lqfp_80::p69>;

int main() {
    // using for the rtc clock
    using rtc_periph = target::io::periph::rtc0;
    using rtc = target::io::rtc<rtc_periph>;

    // using for the button pin
    using button0 = target::io::pin_in<target::pins::package::lqfp_80::p40>;
    using button1 = target::io::pin_in<target::pins::package::lqfp_80::p39>;
    using button2 = target::io::pin_in<target::pins::package::lqfp_80::p38>;

    using fat_helper = menu::detail::fat_helper;

    // using for the usb driver/device
    using usb_keyboard = target::io::usb<target::io::periph::lqfp_80::usb0, klib::usb::device::keyboard_hid<4>>;
    using usb_mouse = target::io::usb<target::io::periph::lqfp_80::usb0, klib::usb::device::mouse_hid<4>>;
    using usb_massstorage = target::io::usb<target::io::periph::lqfp_80::usb0, 
        klib::usb::device::mass_storage<fat_helper>
    >;
    
    // init all the display hardware and the buttons
    ssp::init<klib::io::spi::mode::mode3, 48'000'000, klib::io::spi::bits::bit_8, true>();

    // init the io needed for the display
    blk::init();
    rst::init();
    dc::init();
    cs::init();

    // enable the backlight and clear the chip select to 
    // the display so we can talk to it
    blk::set<false>();
    cs::set<false>();

    // init the dma engine
    using dma_periph = target::io::periph::dma0;
    using dma = target::io::dma<dma_periph>;
    dma::init<false>();

    // create the dma channels
    using dma_tx = target::io::dma_channel<dma_periph, 0, klib::io::dma::memory, ssp>;
    using dma_rx = target::io::dma_channel<dma_periph, 1, ssp, klib::io::dma::memory>;

    dma_tx::init();
    dma_rx::init();

    using display = klib::hardware::display::st7789_dma<dma_tx, dma_rx, ssp, dc, rst, klib::graphics::mode::rgb565, 240, 135, 40, 52>;

    // init the buttons
    button0::init();
    button1::init();
    button2::init();

    // check if any of the buttons is pressed
    const bool flipped = button0::get() || button2::get();

    // initialize the display in the correct orientation. We flip the screen
    // if any of the buttons are pressed while we are powered on
    if (flipped) {
        display::init<true, false, klib::graphics::orientation::landscape, false, true>();
    }
    else {
        display::init<true, false, klib::graphics::orientation::landscape, true, false>();
    }

    // clear any data left in the fifo register to prevent 
    // stalls when we use the dma later
    ssp::clear_rx_fifo();

    // create the framebuffers
    constexpr static uint32_t move_height = 10;
    using fb_t = klib::graphics::movable_framebuffer<display, display::mode, 0, 0, display::width, move_height, std::endian::big>;

    // this needs to be static to move it to RAM1. 
    static std::array<fb_t, 2> framebuffer __attribute__ ((section(".framebuffer"))) = {fb_t(), fb_t()};

    // init all the framebuffers
    for (auto& fb: framebuffer) {
        fb.init();
    }

    // setup the usb pll
    target::io::system::clock::set_usb<12'000'000>();

    // using for writing to flash
    using flash = target::io::flash;

    // using for the storage
    using storage = storage::storage<flash>;

    // create the popup first as some other screens use it
    menu::numeric_popup<fb_t> numeric_popup = {};
    menu::popup<fb_t> string_popup = {};

    // setup the first state. We initialize everything 
    // except the screen in the splash screen. This speeds
    // up the boot time and will show the splash screen
    // until we are done initializing. 
    menu::splash<fb_t, storage, rtc, usb_keyboard> splash = {};
    menu::totp<fb_t, storage, rtc, rtc_periph, usb_keyboard> totp = {};
    menu::settings<fb_t> settings = {};
    menu::time<fb_t, rtc_periph, rtc> time(numeric_popup);
    menu::calibration<fb_t, rtc_periph> calibration(numeric_popup, string_popup);
    menu::config<
        fb_t, storage, fat_helper, 
        usb_keyboard, usb_massstorage
    > config = {};
    menu::mouse<fb_t, usb_keyboard, usb_mouse> mouse = {};

    // array with all the app screens
    menu::screen<fb_t> *const screens[] = {
        &splash,
        &totp,
        &settings,
        &time,
        &calibration,
        &config,
        &mouse,
        &numeric_popup,
        &string_popup,
    };

    // get the current screen
    uint8_t previous_screen = static_cast<uint8_t>(menu::screen<fb_t>::get());

    // call activate on the first screen
    screens[previous_screen]->activate(menu::screen_id::splash);

    // current active framebuffer
    uint8_t current_framebuffer = 0;

    // last time the user pressed a button for the screen timeout
    auto last_pressed_time = klib::io::systick<>::template get_runtime();

    // get the previous time for the delta
    auto previous_time = klib::io::systick<>::get_runtime<klib::time::us>();

    // timing for the buttons. To keep track on how long a 
    // button is pressed. We mark everything with the pressed
    // marker to make sure we do not enter a menu straight 
    // away
    std::array<klib::time::us, 3> button_timing = {
        input::pressed_marker, input::pressed_marker, 
        input::pressed_marker
    };

    while (true) {
        // get the current screen
        const uint8_t current_screen = static_cast<uint8_t>(menu::screen<fb_t>::get());

        // get the current time
        const auto current_time = klib::io::systick<>::template get_runtime<klib::time::us>();

        // get the buttons
        input::buttons buttons = input::get_state(
            current_time - previous_time, flipped, button_timing, 
            {button0::get(), button1::get(), button2::get()}
        );

        // flag if we have reached the screen timeout
        const bool timeout = klib::io::systick<>::get_runtime() > (last_pressed_time + screen_timeout);

        // check if we have pressed any button
        if (is_pressed(buttons.up) || is_pressed(buttons.enter) || is_pressed(buttons.down)) {
            // check if we need to enable the backlight again
            if (timeout) {
                // turn on the backlight
                blk::set<false>();

                // prevent any of the buttons from triggering when we turn on the backlight
                buttons = {input::state::no_change, input::state::no_change, input::state::no_change};
            }

            // update the last time we pressed the buttons
            last_pressed_time = klib::io::systick<>::get_runtime();
        }
        else if (timeout) {
            // turn off the backlight
            blk::set<true>();
        }

        // check if we have switched screens
        if (current_screen != previous_screen) {
            // deactivate the previous screen
            screens[previous_screen]->deactivate(
                static_cast<menu::screen_id>(current_screen)
            );

            // activate the current screen
            screens[current_screen]->activate(
                static_cast<menu::screen_id>(previous_screen)
            );

            // set the current screen as the previous screen for a next change
            previous_screen = current_screen;

            // clear the delta for the current screen
            previous_time = current_time;
        }

        // run the correct screen
        screens[current_screen]->main(current_time - previous_time, buttons);

        // draw the screen alternating the framebuffers
        for (uint32_t i = 0; i < display::height; i += move_height) {
            // call the on_draw
            screens[current_screen]->draw(
                framebuffer[current_framebuffer], {0, i}
            );

            // wait until the previous segment is done until we update it
            while (dma_tx::is_busy()) {
                // wait
            }

            // after the dma signals it is done we still have data in the
            // ssp fifo. Wait until we are done with that as well before
            // we write new data
            while (ssp::is_busy()) {
                // wait
            }

            // clear any data left in the fifo register to prevent 
            // stalls when we use the dma later
            ssp::clear_rx_fifo();

            // flush the framebuffer to the display
            framebuffer[current_framebuffer].flush(klib::vector2u(0, i));

            // swap the framebuffer we are using
            current_framebuffer ^= 1;
        }

        // update the previous time
        previous_time = current_time;

        const auto end_time = klib::io::systick<>::template get_runtime<klib::time::us>();

        // we try to target around 60 fps
        if ((end_time - current_time).value < fps_frametime) {
            // wait the remaining time
            klib::delay(klib::time::us(fps_frametime - (end_time - current_time).value));
        }
    }
}

namespace fault {
    /**
     * @brief Available fault types
     * 
     */
    enum class type {
        // vector table entries
        hardfault = 3,
        memfault = 4,
        busfault = 5,
        usagefault = 6,

        // other fault types. Note: value is outside the 
        // vector table range
        functioncall = 256,
    };

    /**
     * @brief Hardfault handler that shows a error on the screen
     * this function initializes the hardware of the displays and
     * uses a direct framebuffer to prevent using too much ram and
     * causing another issue
     * 
     * @note noinline attribute is to prevent the compiler from 
     * inlining this in the naked function. When that happens
     * the error screen does not popup for some reason. 
     * 
     * TODO: figure out why this happens
     * 
     * @param fault 
     * @param stack 
     */
    void __attribute__ ((noinline)) hardfault_handler_impl(const type fault, const uint32_t stack) {
        // create a display without dma
        // TODO: make this sync up with the other display somehow
        using display = klib::hardware::display::st7789<ssp, dc, rst, klib::graphics::mode::rgb565, 240, 135, 40, 52>;

        // reinit the hardware needed to show something on the display
        ssp::template init<klib::io::spi::mode::mode3, 48'000'000, klib::io::spi::bits::bit_8, true>();

        // clear the rx fifo just in case there is data left in the fifo after the crash
        ssp::clear_rx_fifo();

        // init the io needed for the display
        blk::init();
        rst::init();
        dc::init();
        cs::init();

        // enable the backlight and clear the chip select to 
        // the display so we can talk to it
        blk::set<false>();
        cs::set<false>();

        // just init the screen, no need to flip it
        display::init<true, false, klib::graphics::orientation::landscape, true, false>();

        // write directly to the display
        using framebuffer_t = klib::graphics::direct_framebuffer<display, true, 0, 0, display::width, display::height, std::endian::little>;
        framebuffer_t framebuffer;

        // initialize the framebuffer
        framebuffer.init();

        // clear the framebuffer
        framebuffer.clear(klib::graphics::yellow);

        // write the framebuffer to the display
        framebuffer.flush();

        // fonts to display something on the screen
        using small_font = klib::graphics::ascii_font_8x8;
        using small_text = klib::graphics::string<small_font>;
        using large_font = klib::graphics::ascii_font_16x16;
        using large_text = klib::graphics::string<large_font>;

        constexpr static char title[] = "Crash detected";

        // write we detected a crash
        large_text::draw(framebuffer, title, 
            klib::vector2i{
                (240 - static_cast<int32_t>(klib::string::strlen(title) * large_text::font::width)) / 2, 3
            }, klib::graphics::black, klib::graphics::transparent
        );

        // string to show
        char buffer[32] = {};

        switch (fault) {
            case type::functioncall: 
                klib::string::strcpy(buffer, "Function fault");
                break;
            default:
                // check if we have a stack overflow
                if ((stack <= reinterpret_cast<uint32_t>(&__stack_start))|| (stack > reinterpret_cast<uint32_t>(&__stack_end))) {
                    // copy the string to the buffer
                    klib::string::strcpy(buffer, "Stack overflow SP:");

                    // copy the stack we crashed on to the buffer
                    klib::string::itoa<klib::base::HEX>(stack, buffer + klib::string::strlen(buffer));
                }
                else {
                    // copy the string to the buffer
                    klib::string::strcpy(buffer, "Unknown fault:");
                    klib::string::itoa(static_cast<uint32_t>(fault), buffer + klib::string::strlen(buffer));
                }
                break;
        }

        // check if we need to show the additional line
        if (klib::string::strlen(buffer)) {
            // draw the stack pointer we detected
            small_text::draw(
                framebuffer, buffer,
                klib::vector2i{
                    (240 - static_cast<int32_t>(klib::string::strlen(buffer) * small_text::font::width)) / 2, 40
                }, 
                klib::graphics::black, klib::graphics::transparent
            );
        }

        // message to display
        constexpr static char message[] = 
            "Unplug authenticator\nand replug USB to restart.\n\nIf you keep seeing this error\n"
            "please report this error to:\n\nkoon.io/totp\n";
        
        uint32_t start = 0;
        uint32_t count = 0;

        // display the strings
        for (uint32_t i = 0; i < klib::string::strlen(message); i++) {
            if (message[i] != '\n') {
                continue;
            }

            // draw the text
            small_text::draw(
                framebuffer, 
                &message[start], (i - start),
                klib::vector2i{
                    (240 - static_cast<int32_t>((i - start) * small_text::font::width)) / 2,
                    static_cast<int32_t>(56 + (small_text::font::height * count))
                }, 
                klib::graphics::black, klib::graphics::transparent
            );

            // update the start
            start = i + 1;
            count++;
        }

        // write the framebuffer to the display
        framebuffer.flush();
    }
}

/**
 * @brief Hardfault handler. Called when we crashed
 * 
 */
void __attribute__((__noreturn__, __naked__)) hardfault_handler() {
    // get the current sp register. Note: this cannot be on the stack
    // the stack might not be valid at this point
    const uint32_t sp = __get_MSP();

    // make sure we have a valid stack pointer. At this point we are gonna 
    // overwrite everything as we cannot recover anyway. (the heap is at
    // the end of everything)
    __set_MSP(reinterpret_cast<uint32_t>(&__heap_end));

    // call the hardfault implementation after we have changed the stack pointer
    fault::hardfault_handler_impl(static_cast<fault::type>(__get_IPSR()), sp);

    // wait until the user unplugs
    while (true) {
        // do nothing
    }
}

// implement the throw_bad_function_call so we can use std::function
namespace std {
    void __throw_bad_function_call() { 
        // call the hardfault implementation if we ever get here
        fault::hardfault_handler_impl(
            fault::type::functioncall, __get_MSP()
        );

        while(true) {
            // do nothing when we have a bad function call
        }
    }; 
}

/**
 * @brief Constructor that gets called before main is called. This 
 * constructor registers the hardfault_handler as the callback for
 * every fault we can detect
 * 
 */
void __attribute__((__constructor__)) fault_startup() {
    // register the hard fault handler for all the faults we 
    // can detect before we run main. This is so we can catch 
    // everything main does
    target::irq::register_irq<target::irq::arm_vector::hard_fault>(hardfault_handler);
    target::irq::register_irq<target::irq::arm_vector::memory_managagement_fault>(hardfault_handler);
    target::irq::register_irq<target::irq::arm_vector::bus_fault>(hardfault_handler);
    target::irq::register_irq<target::irq::arm_vector::usage_fault>(hardfault_handler);
}