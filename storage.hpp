#pragma once

#include <span>
#include <cstdint>

#include <klib/dynamic_array.hpp>
#include <klib/string.hpp>

namespace storage {
    enum class digit: uint8_t {
        digits_6 = 6,
        digits_8 = 8
    };

    /**
     * @brief Returns if a digit is valid
     * 
     * @param d 
     * @return true 
     * @return false 
     */
    constexpr bool is_valid(const digit d) {
        return d == digit::digits_6 || d == digit::digits_8;
    }

    /**
     * @brief Totp entry
     * 
     */
    struct entry {
        // name to be displayed above the code. Limited
        // to 14 characters + null terminator (more does
        // not fit on the screen)
        char str[15];

        // amount of digits shown on the screen
        digit digits;

        // interval to change the key
        uint8_t interval;

        // padding
        uint8_t padding[3];

        // key (we allow up to 320 bits, sha1 will "compress" it to 
        // 160 bits if longer and zero extend it when shorter)
        klib::dynamic_array<char, 40> key;
    };

    static_assert(sizeof(entry) == 64, "Invalid size");

    /**
     * @brief Storage class that reads the entries
     * 
     */
    template <typename Flash, uint32_t Start, uint32_t End>
    class storage {
    public:
        // the max amount of entries we support for now
        constexpr static uint32_t max_entries = 32;

    protected:
        // array to store all the entries
        static inline klib::dynamic_array<entry, max_entries> entries = {};

    public:
        /**
         * @brief Read the memory and add them to the entries array
         * 
         * @param start 
         * @param end 
         * @param key 
         */
        static void init(const std::span<uint8_t> key) {
            uint32_t address = Start;

            // get all the entries
            for (uint32_t i = 0; (i < max_entries) && ((address + sizeof(entry)) < End); i++) {
                // TODO: decrypt the keys here
                const auto& e = *reinterpret_cast<const entry*>(address);

                // check if the current entry is valid (we check for a empty 
                // string or all 0xff)
                if (e.str[0] == 0xff || !is_valid(e.digits) || 
                    !e.interval)
                {
                    // we are at the end or we have a corrupt entry. Exit
                    break;
                }

                // copy to the local array
                entries.push_back(e);

                // move to the next entry
                address += sizeof(entry);
            }
        }

        /**
         * @brief Get all the entries we have stored
         * 
         * @return std::span<entry> 
         */
        static klib::dynamic_array<entry, max_entries>& get_entries() {
            return entries;
        }

        /**
         * @brief Writes the current entries to flash memory
         * 
         */
        static void write() {
            // erase the sector
            Flash::erase(Flash::erase_mode::sector, Start);

            // write in 1024 byte chunks
            for (uint32_t i = 0; i < ((max_entries * sizeof(entry)) / 1024); i++) {
                const std::span<const uint8_t> p = {
                    (reinterpret_cast<const uint8_t*>(entries.data()) + i * 1024), 1024
                };

                // write the new data
                Flash::write(Start + (i * 1024), p);
            }
        }
    };
}