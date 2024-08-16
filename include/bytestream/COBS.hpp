//
// Created by Obi Davis on 07/03/2024.
//

#ifndef COBS_HPP
#define COBS_HPP

#include <type_traits>
#include <iterator>
#include <optional>

template <typename InputIt, typename OutputIt>
constexpr OutputIt cobs_encode_frame(InputIt first, InputIt last, OutputIt output) {
    static_assert(std::is_integral<typename std::iterator_traits<InputIt>::value_type>::value,
        "InputIt must have an integral value type");
    static_assert(std::is_integral<typename std::iterator_traits<OutputIt>::value_type>::value,
        "OutputIt must have an integral value type");
    static_assert(std::is_same<typename std::iterator_traits<OutputIt>::iterator_category, std::random_access_iterator_tag>::value,
        "OutputIt must be a random access iterator");

    using byte_type = typename std::iterator_traits<OutputIt>::value_type;

    auto block_start = output++;
    byte_type block_length = 1;

    for (; first != last; ++first) {
        if (*first) {
            *output++ = *first;
            block_length++;
        }
        if (*first == 0 || block_length == 0xFF) {
            *block_start = block_length;
            block_start = output;
            block_length = 1;
            if (*first == 0 || std::distance(first, last) != 1) {
                ++output;
            }
        }
    }
    *block_start = block_length;
    *output++ = 0;
    return output;
}

template <typename InputIt, typename OutputIt>
constexpr size_t cobs_encode_frame(InputIt first, size_t size, OutputIt output) {
    return std::distance(output, cobs_encode_frame(first, first + size, output));
}

template <typename InputIt, typename OutputIt>
constexpr OutputIt cobs_decode_frame(InputIt first, InputIt last, OutputIt output) {
    using byte_type = typename std::iterator_traits<InputIt>::value_type;

    for (byte_type code = 0xFF, block = 0; first != last; --block) {
        if (block) {
            *output++ = *first++;
        } else {
            block = *first++;
            if (block && (code != 0xFF)) {
                *output++ = 0;
            }
            code = block;
            if (!code) {
                break;
            }
        }
    }
    return output;
}

template <typename InputIt, typename OutputIt>
constexpr size_t cobs_decode_frame(InputIt first, size_t size, OutputIt output) {
    return std::distance(output, cobs_decode_frame(first, first + size, output));
}

class COBSDecoder {
    uint8_t block_remaining{0};
    uint8_t code{0xFF};
    bool packet_complete_flag{false};
public:
    template <typename Byte>
    [[nodiscard]] constexpr std::optional<Byte> process_byte(Byte byte) {
        if (packet_complete()) {
            reset();
        }

        if (block_remaining) {
            --block_remaining;
            return std::optional<Byte>{byte};
        }

        block_remaining = byte;
        if (block_remaining && code != 0xFF) {
            code = block_remaining--;
            return std::optional<Byte>{0};
        }

        code = block_remaining--;
        if (!code) {
            packet_complete_flag = true;
        }

        return std::nullopt;
    }
    // Returns true if the byte was a data byte
    template <typename Byte>
    [[nodiscard]] constexpr bool process_byte(Byte byte, Byte *output) {
        if (packet_complete()) {
            reset();
        }

        if (block_remaining) {
            *output = byte;
            --block_remaining;
            return true;
        }

        block_remaining = byte;
        if (block_remaining && code != 0xFF) {
            *output = 0;
            code = block_remaining--;
            return true;
        }

        code = block_remaining--;
        if (!code) {
            packet_complete_flag = true;
        }

        return false;
    }


    [[nodiscard]] constexpr bool packet_complete() const {
        return packet_complete_flag;
    }

    constexpr void reset() {
        block_remaining = 0;
        code = 0xFF;
        packet_complete_flag = false;
    }
};

[[nodiscard]] constexpr size_t cobs_encoded_max_length(size_t length) {
    return 2 + length + (length + 253) / 254;
}


#endif //COBS_HPP