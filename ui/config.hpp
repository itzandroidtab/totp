#pragma once

#include <cstring>
#include <optional>

#include <klib/filesystem/virtual_fat.hpp>
#include <klib/delay.hpp>
#include <klib/string.hpp>
#include <klib/crypt/base32.hpp>

#include "screen.hpp"

namespace menu::detail {
    const char* strnstr(const char *haystack, const char *needle, size_t len) {
        const size_t needle_len = klib::string::strlen(needle);

        if (!needle_len) {
            return haystack;
        }

        for (uint32_t i = 0; i <= (len - needle_len); i++) {
            if ((haystack[0] == needle[0]) && (0 == std::strncmp(haystack, needle, needle_len))) {
                return haystack;
            }

            haystack++;
        }

        return nullptr;
    }

    class fat_helper {
    public:
        // fat filesystem
        using filesystem = klib::filesystem::virtual_fat<fat_helper, 16, (static_cast<uint32_t>(1) * 1024 * 1024), 16, 1>;

    protected:
        // current file that is being processed
        static inline const klib::filesystem::detail::directory* file = 0;

        // offset to the first sector of the file
        static inline uint32_t start_offset = 0;

        // amount of bytes received
        static inline uint32_t transferred_bytes = 0;

    public:
        /**
         * @brief Functions required for the filesystem. These functions will 
         * be called when something changes in the filesystem
         * 
         */

        static void on_delete(const uint32_t index, const klib::filesystem::detail::directory& old) {
            // do nothing
        }

        static void on_create(const uint32_t index, const klib::filesystem::detail::directory& n) {
            // do nothing
        }

        static void on_change(const uint32_t index, const klib::filesystem::detail::directory& old, const klib::filesystem::detail::directory& n) {
            // do nothing
        }

        static void file_handler(const uint32_t offset, const uint8_t *const data, const uint32_t sectors) {
            // do nothing
        }

    public:
        /**
         * @brief Functions required for the mass storage interface. Connects 
         * the virtual filesystem and the interface together
         * 
         */

        /**
         * @brief Init the memory
         * 
         */
        static void init() {
            // do nothing here
        }

        /**
         * @brief Start the memory
         * 
         */
        static void start() {
            // nothing to do here
        }

        /**
         * @brief Stop any pending transaction and flush to disk
         * 
         * @return true 
         * @return false 
         */
        static bool stop() {
            return true;
        }

        /**
         * @brief Return if the memory is ready
         * 
         * @return true 
         * @return false 
         */
        static bool ready() {
            return true;
        }

        /**
         * @brief Returns if the device can be removed from the host
         * 
         * @return true 
         * @return false 
         */
        static bool can_remove() {
            // return the device can be removed
            return true;
        }

        /**
         * @brief Returns the size of the memory device in bytes
         * 
         * @return uint32_t 
         */
        static uint32_t size() {
            // return the filesystem size
            return filesystem::size();
        }

        /**
         * @brief Returns if the drive is writable
         * 
         * @return true 
         * @return false 
         */
        static bool is_writable() {
            return true;
        }

        /**
         * @brief Read a sector. Can read up to SectorSize of bytes 
         * at once (without crossing a sector boundry)
         * 
         * @param data 
         * @param address 
         * @param size 
         * @return true 
         * @return false 
         */
        static bool read(uint8_t *const data, const uint32_t address, const uint16_t size) {
            // change the address and size to sector sizes and give it to the virtual filesystem
            filesystem::read(address / filesystem::sector_size, data, size / filesystem::sector_size);

            // return we are good
            return true;
        }

        /**
         * @brief Write to a sector. Can write up to SectorSize of bytes 
         * at once (without crossing a sector boundry)
         * 
         * @param data 
         * @param address 
         * @param size 
         * @return true 
         * @return false 
         */
        static bool write(uint8_t *const data, const uint32_t address, const uint16_t size) {
            // change the address and size to sector sizes and give it to the virtual filesystem
            filesystem::write(address / filesystem::sector_size, data, size / filesystem::sector_size);

            // return we are good
            return true;
        }
    };
}

namespace menu {
    template <
        typename FrameBuffer, typename Storage, typename FatHelper, 
        typename UsbKeyboard, typename UsbMassStorage
    >
    class config: public screen<FrameBuffer> {
    protected:
        using screen_base = screen<FrameBuffer>;

        // readme for the config file.
        constexpr static char config_header[] = 
            "CSV profile editor for KLIB TOTP\r\n\r\nThis file allows you to delete/create new profiles. "
            "To create a new profile, add a new line below the CSV header in the following format:\r\n\r\n"
            "profile name, interval, digits, key in hex or in string format.\r\n\r\nExample:\r\n"
            "test, 30, 6, \"SOMEKEY123\"\r\ntest, 30, 6, 0xab 0xcd 0xef 0x12 0x34 0x56 0x78 0x90\r\n"
            "test, 30, 6, abcdef1234567890\r\ntest, 30, 8, b32\"MFRGGZDFMYYTEMZUGU3DOOBZGA======\""
            "\r\n\r\nprofile, interval, digits, key\r\n";

        constexpr static char config_csv[] = "profile, interval, digits, key\r\n";

        // end of file marker used to determine 
        // when to write the entries to flash
        constexpr static char config_end[] = "EOF\r\n";

        /**
         * @brief Get the line length of a entry
         * 
         * @return uint32_t 
         */
        constexpr static uint32_t get_entry_length(const storage::entry& entry) {
            // format of a single line: profile: "str", interval: digits, digits: digits, key: ***
            return (
                klib::string::strlen(", , , ***\r\n") +
                klib::string::strlen(entry.str) + klib::string::detail::count_chars(entry.interval) +
                klib::string::detail::count_chars(static_cast<uint8_t>(entry.digits))
            );
        }

        /**
         * @brief Writes entry data in the provided buf
         * 
         * @param entry 
         * @param buf 
         */
        constexpr static void write_entry(const storage::entry& entry, char *const buf) {
            klib::string::strcpy(buf, entry.str);
            klib::string::strcat(buf, ", ");
            klib::string::itoa(entry.interval, buf + klib::string::strlen(buf));
            klib::string::strcat(buf, ", ");
            klib::string::itoa(static_cast<uint8_t>(entry.digits), buf + klib::string::strlen(buf));
            klib::string::strcat(buf, ", ***\r\n");
        }

        // buffer to store a single line
        static inline klib::dynamic_array<char, FatHelper::filesystem::sector_size> buffer;

        /**
         * @brief Read file implementation
         * 
         * @param offset 
         * @param data 
         * @param sectors 
         */
        static void read_config(const uint32_t offset, uint8_t *const data, const uint32_t sectors) {
            // check if we have anything to do. No sectors is do nothing
            if (!sectors) {
                return;
            }

            // amount of bytes we should write
            uint32_t bytes = sectors * FatHelper::filesystem::sector_size;
            uint32_t start_byte = (offset * FatHelper::filesystem::sector_size);

            uint32_t current_byte = 0;
            uint32_t byte_offset = 0;

            // get all the entries
            const auto entries = Storage::get_entries();

            // check what media should handle the read
            for (uint32_t i = 0; i < entries.size() + 2; i++) {
                // get the amount of bytes for the current entry
                const uint32_t entry_bytes = ((i == 0) ? (sizeof(config_header) - 1) : 
                    ((i == (entries.size() + 1)) ? (sizeof(config_end) - 1) : get_entry_length(entries[i - 1]))
                );

                // get the end of the current media
                const uint32_t current_end = current_byte + entry_bytes;
                
                // check if the request is for the current media
                if ((start_byte >= current_byte) && (start_byte < current_end)) {
                    // get the offset in the current media
                    const uint32_t media_offset = start_byte - current_byte;

                    // get the maximum amount we should read in the current media
                    const uint32_t count = klib::min(current_end - start_byte, bytes);

                    if (i == 0) {
                        // copy the header into the buffer
                        std::copy_n(config_header + media_offset, count, &data[byte_offset]);
                    }
                    else if (i == (entries.size() + 1)) {
                        // copy the header into the buffer
                        std::copy_n(config_end + media_offset, count, &data[byte_offset]);
                    }
                    else {
                        // check if the entry fits in the buffer
                        if (entry_bytes < buffer.max_size()) {
                            // read the current entry into the buffer
                            write_entry(entries[i - 1], buffer.data());

                            // copy the data to the buffer we are sending to the host
                            std::copy_n(buffer.data() + media_offset, count, &data[byte_offset]);
                        }
                    }

                    // update the sector and the amount of sectors we want to process
                    bytes -= count;
                    start_byte += count;

                    // move the offset we are in the ptr
                    byte_offset += count;
                }

                // check if we have data left to process
                if (!sectors) {
                    return;
                }

                // move past the current media to go to the next one
                current_byte += entry_bytes;
            }
        }

        /**
         * @brief Returns if range is a number or a space
         * 
         * @param data 
         * @param length 
         * @return true 
         * @return false 
         */
        static bool is_number_range(const char* data, const uint32_t length) {
            // make sure every character is a number or a space
            for (uint32_t b = 0; b < length; b++) {
                // get the current character
                const auto ch = data[b];

                // check if a number or space
                if (!klib::string::is_digit(ch) && ch != ' ') {
                    return false;
                }
            }

            return true;
        }

        enum class parse_result {
            invalid,
            unchanged,
            valid,
        };

        /**
         * @brief Parse the key portion of the ascii data
         * 
         * @param entry 
         * @param data 
         * @param length 
         * @return parse_result 
         */
        static parse_result parse_key(storage::entry& entry, const char* data, const uint32_t length) {
            // count the amount of spaces before the first character
            const auto* ptr = std::find_if(data, data + length, [](const uint8_t ch) { return ch != ' '; });

            constexpr static char unchanged[] = "***";

            // check for a unchanged item
            if (std::strncmp(ptr, unchanged, (sizeof(unchanged) - 1)) == 0) {
                return parse_result::unchanged;
            }

            // parse the key. The key is till the end of the line. We can either have 
            // a hex array, string or a base32 string. Hex string needs to have "" around
            // it. Search for the ""
            const auto quotes = std::count(data, data + length, '"');

            // check if we have a valid entry
            if (quotes > 0 && quotes != 2) {
                return parse_result::invalid;
            }

            if (quotes) {
                // check if have a base32 string or a hex string
                const auto start = std::find(data, data + length, '"');
                const auto end = std::find(start + 1, data + length, '"');

                // check for base 32
                if ((std::strncmp((start - 3), "b32", 3) == 0) || 
                    (std::strncmp((start - 3), "B32", 3) == 0)) 
                {
                    // support up to 320 bits on the input and output 
                    // input = 320 / 5 = 64 bytes
                    // output = 320 / 8 = 40 bytes
                    char input[65] = {};
                    uint8_t buf[40];

                    // check if we can copy it to the local buffer
                    if ((end - (start + 1)) >= (sizeof(input) - 1)) {
                        return parse_result::invalid;
                    }

                    // copy to the local buffer
                    std::copy_n((start + 1), end - (start + 1), input);

                    // decode the base32 value
                    const uint32_t size = klib::crypt::base32::decode(input, buf);

                    // check if the conversion was successfull
                    if (!size) {
                        return parse_result::invalid;
                    }

                    // copy the bytes to the key
                    for (uint32_t i = 0; i < size; i++) {
                        entry.key.push_back(buf[i]);
                    }
                }
                else {
                    // we are in string mode. copy the data directly 
                    // into our buffer if it fits
                    if ((end - (start + 1)) >= entry.key.max_size()) {
                        return parse_result::invalid;
                    }

                    // add all the characters
                    for (uint32_t i = 0; i < (end - (start + 1)); i++) {
                        entry.key.push_back(start[1 + i]);
                    }
                }

                return parse_result::valid;
            }
            else {
                // we support the following formats:
                // - "0xXX"
                // - "XX"
                // where XX are the 2 nibbles.
                klib::dynamic_array<char, 8> buf = {};

                // we do a lookup on the previous item. So we start at index 1
                for (uint32_t i = 0; i < length; i++) {
                    // parse the current character
                    const auto ch = data[i];
                    
                    // check for valid characters
                    if (ch == ' ') {
                        // skip spaces
                        continue;
                    }

                    // check if we have a hex prefix we need to remove
                    if (data[i] == '0' && ((i + 1) < length) && klib::string::to_lower(data[i + 1]) == 'x') {
                        // skip the current and next character
                        i++;

                        continue;
                    }

                    if (klib::string::is_hex(ch)) {
                        // add to the buffer
                        buf.push_back(ch);

                        // check if we have enough data we can parse
                        if (buf.size() >= 2) {
                            // add the null terminator
                            buf.push_back(0x00);

                            // parse the current buffer
                            const int32_t t = klib::string::stoi<klib::base::HEX>(buf.data());

                            // check for valid numbers and if we still have
                            // room in the buffer
                            if ((t < 0) || (t > 0xff) || (entry.key.size() == entry.key.max_size())) {
                                return parse_result::invalid;
                            }

                            // add it to the key
                            entry.key.push_back(t);

                            // clear the buffer after parsing
                            buf.clear();
                        }
                    }
                    else {
                        // invalid character. Exit
                        return parse_result::invalid;
                    }
                }

                // minimal key length is 8 bytes. Also check if we
                // still have left over data in the buffer
                if (entry.key.size() < 8 || buf.size()) {
                    return parse_result::invalid;
                }

                return parse_result::valid;
            }
        }

        /**
         * @brief Write file implementation. Only works if sectors are in order
         * 
         * @param offset 
         * @param data 
         * @param sectors 
         */
        static void write_config(const uint32_t offset, const uint8_t *const data, const uint32_t sectors) {
            // next sector we are expecting. Reset when we are at offset 0
            static uint32_t next_sector = 0;

            // all the entries that are still valid after the write
            static std::array<bool, Storage::max_entries> valid_entries = {};

            // byte index we are at
            uint32_t index = 0;

            // remove the comment and the csv header from our input
            if (!offset) {
                next_sector = sectors;

                // clear all the valid entries
                valid_entries = {};

                // search for the csv header
                const uint8_t* res = reinterpret_cast<const uint8_t*>(
                    detail::strnstr(
                        reinterpret_cast<const char*>(data), config_csv,
                        sectors * FatHelper::filesystem::sector_size
                    )
                );

                // check if we found the string
                if (res == nullptr) {
                    return;
                }

                // clear any old data left in the buffer
                buffer.clear();

                // move the index
                index += ((res + (sizeof(config_csv) - 1)) - data);
            }
            else {
                // check if we are doing a out of order sector write. We do not support that
                if (offset != next_sector) {
                    // mark as invalid and do nothing until we get a new start
                    next_sector = 0xffffffff;

                    return;
                }
                else {
                    // we have a valid sector. Move the next sector
                    next_sector += sectors;
                }
            }

            // get all the entries
            auto& entries = Storage::get_entries();

            // push every byte we have until a newline
            for (; index < sectors * FatHelper::filesystem::sector_size; index++) {
                // check if we have a newline character
                if (data[index] != '\r' && data[index] != '\n') {
                    // make sure we have enough data in the buffer
                    if (buffer.size() == buffer.max_size()) {
                        buffer.clear();
                    }

                    buffer.push_back(data[index]);

                    continue;
                }

                // check we have at least a few characters in the buffer
                if (buffer.size() < 3) {
                    buffer.clear();

                    continue;
                }

                // check for the end of file (we remove the null terminator + \r\n)
                if (std::strncmp(buffer.data(), config_end, (sizeof(config_end) - 3)) == 0) {
                    // we are at the end of the file. Mark as done
                    next_sector = 0xffffffff;

                    // delete all the entries that are not valid starting from the back
                    for (uint32_t i = entries.size(); i > 0; i--) {
                        if (valid_entries[i - 1]) {
                            continue;
                        }

                        // move everything after the current entry back by one
                        for (uint32_t j = i - 1; j < entries.size(); j++) {
                            entries[j] = entries[j + 1];
                        }

                        // remove the last entry
                        entries.pop_back();
                    }

                    // write the buffer to memory
                    Storage::write();

                    return;
                }

                // we have a line. Parse the line. check if we have enough 
                // commas in the string
                const auto count = std::count(buffer.data(), (buffer.data() + buffer.size()), ',');

                // check for the 3 commas
                if (count != 3) {
                    // invalid, clear the buffer and go to the next one
                    buffer.clear();

                    continue;
                }

                // temporary storage for a single entry
                storage::entry ret = {};

                // we have the amount of items we expect. Check if the lengths are 
                // correct for the strings
                const auto str_end = std::find(buffer.data(), (buffer.data() + buffer.size()), ',');

                // check for the size
                if ((str_end - buffer.data()) >= sizeof(storage::entry::str)) {
                    // invalid, clear the buffer and go to the next one
                    buffer.clear();

                    continue;
                }

                // copy the data in the entry buffer
                std::copy_n(buffer.data(), (str_end - buffer.data()), ret.str);

                // get the length of the interval
                char* interval_end = std::find(str_end + 1, (buffer.data() + buffer.size()), ',');

                // check if all the characters in the item are numbers or spaces
                if (!is_number_range(str_end + 1, (interval_end - (str_end + 1)))) {
                    // invalid, clear the buffer and go to the next one
                    buffer.clear();

                    continue;
                }

                // change the comma to a null terminator for stoi
                (*interval_end) = 0x00;

                // convert the string to a number
                ret.interval = klib::string::stoi(str_end + 1);

                // get the length of the interval
                auto digits_end = std::find(interval_end + 1, (buffer.data() + buffer.size()), ',');

                // check if all the characters in the item are numbers or spaces
                if (!is_number_range(interval_end + 1, (digits_end - (interval_end + 1)))) {
                    // invalid, clear the buffer and go to the next one
                    buffer.clear();

                    continue;
                }

                // change the comma to a null terminator for stoi
                (*digits_end) = 0x00;

                // convert the string to a number
                ret.digits = static_cast<storage::digit>(klib::string::stoi(interval_end + 1));

                if (!storage::is_valid(ret.digits)) {
                    // invalid, clear the buffer and go to the next one
                    buffer.clear();

                    continue;
                }

                // parse the key
                const auto res = parse_key(ret, digits_end + 1, (buffer.data() + buffer.size()) - (digits_end + 1));

                // add the entry if we dont have it already
                if (res == parse_result::valid) {
                    const uint32_t size = entries.size();

                    if (size >= entries.max_size()) {
                        // buffer is full. Do not write to it
                        buffer.clear();

                        continue;
                    }

                    // mark the entry as valid
                    valid_entries[size] = true;

                    // add our entry to the entries
                    entries.push_back(ret);
                }
                else if (res == parse_result::unchanged) {
                    // search what entry this is
                    for (uint32_t i = 0; i < entries.size(); i++) {
                        if (std::strncmp(entries[i].str, ret.str, sizeof(storage::entry::str) - 1) != 0) {
                            continue;
                        }

                        // we have a match
                        valid_entries[i] = true;

                        break;
                    } 
                }

                // we parsed the data clear it for the next line
                buffer.clear();
            }
        }

    public:
        config() {}

        virtual void activate(const screen_id id) override {
            // disconnect the usb keyboard from the host
            // this prevents a blue screen when we suddenly
            // change to a different usb type
            UsbKeyboard::disconnect();

            // wait a bit to give the host time to detect it
            klib::delay<>(klib::time::ms(500));

            // initialize the filesystem
            FatHelper::filesystem::init("KLIB");

            // get the size of the file
            const auto entries = Storage::get_entries();
            uint32_t length = 0;

            for (uint32_t i = 0; i < entries.size(); i++) {
                // increment the length for the total file size
                length += get_entry_length(entries[i]);
            }

            // add the header size
            length += (sizeof(config_header) - 1) + (sizeof(config_end) - 1);

            // create a readme file
            FatHelper::filesystem::create_file("CONFIG  TXT", length, read_config, write_config);  
            
            // initialize the usb mass storage
            UsbMassStorage::init();
        }

        virtual void deactivate(const screen_id id) override {
            // disconnect the usb mass storage from the host
            // this prevents a blue screen when we suddenly
            // change to a different usb type
            UsbMassStorage::disconnect();

            // wait a bit to give the host time to detect it
            klib::delay<>(klib::time::ms(500));

            // initialize the usb mass storage
            UsbKeyboard::init();
        }

        virtual void main(const klib::time::us delta, const input::buttons& buttons) override {
            if (buttons.enter == input::state::long_pressed) {
                // go back to the previous screen
                screen_base::buffer.back();
            }
        }

        virtual void draw(FrameBuffer& frame_buffer, const klib::vector2u& offset) override {
            // clear the background black
            frame_buffer.clear(klib::graphics::black);

            // draw the current token using the large font
            screen<FrameBuffer>::large_text::template draw<FrameBuffer>(
                frame_buffer, 
                "USB mode", 
                klib::vector2i{60, 60} - offset.cast<int32_t>(), 
                klib::graphics::white
            );
        }
    };
}