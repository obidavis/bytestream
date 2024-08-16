//
// Created by Obi Davis on 01/07/2024.
//

#ifndef SLIP_HPP
#define SLIP_HPP

#include <cstdint>
#include <iterator>

static constexpr uint8_t SLIP_END = 0xC0;
static constexpr uint8_t SLIP_ESC = 0xDB;
static constexpr uint8_t SLIP_ESC_END = 0xDC;
static constexpr uint8_t SLIP_ESC_ESC = 0xDD;

template <typename InputIt, typename OutputIt>
constexpr OutputIt slip_encode_frame(InputIt first, InputIt last, OutputIt output) {
    for (; first != last; ++first) {
        switch (*first) {
            case SLIP_END:
                *output++ = SLIP_ESC;
                *output++ = SLIP_ESC_END;
                break;
            case SLIP_ESC:
                *output++ = SLIP_ESC;
                *output++ = SLIP_ESC_ESC;
                break;
            default:
                *output++ = *first;
        }
    }
    *output++ = SLIP_END;
    return output;
}

template <typename InputIt, typename OutputIt>
constexpr size_t slip_encode_frame(InputIt first, size_t size, OutputIt output) {
    return std::distance(output, slip_encode_frame(first, first + size, output));
}

template <typename InputIt, typename OutputIt>
constexpr OutputIt slip_decode_frame(InputIt first, InputIt last, OutputIt output) {
    bool escaped = false;
    for (; first != last; ++first) {
        switch (*first) {
            case SLIP_END:
                return output;
            case SLIP_ESC:
                escaped = true;
                break;
            case SLIP_ESC_END:
                if (escaped) {
                    *output++ = SLIP_END;
                    escaped = false;
                } else {
                    *output++ = SLIP_ESC_END;
                }
                break;
            case SLIP_ESC_ESC:
                if (escaped) {
                    *output++ = SLIP_ESC;
                    escaped = false;
                } else {
                    *output++ = SLIP_ESC_ESC;
                }
                break;
            default:
                *output++ = *first;
        }
    }
    return output;
}

template <typename InputIt, typename OutputIt>
constexpr size_t slip_decode_frame(InputIt first, size_t size, OutputIt output) {
    return std::distance(output, slip_decode_frame(first, first + size, output));
}

[[nodiscard]] constexpr size_t slip_encoded_max_length(size_t length) {
    return length * 2 + 2;
}

class SLIPDecoder {
    bool escaped;
    bool packet_complete_flag;
public:
    SLIPDecoder() : escaped(false) {}

    template <typename Byte>
    [[nodiscard]] constexpr bool process_byte(Byte byte, Byte *output) {
        if (packet_complete()) {
            reset();
        }
        switch (byte) {
            case SLIP_ESC:
                escaped = true;
                return false;
            case SLIP_ESC_END:
                if (escaped) {
                    *output = SLIP_END;
                    escaped = false;
                } else {
                    *output = SLIP_ESC_END;
                }
                return true;
            case SLIP_ESC_ESC:
                if (escaped) {
                    *output = SLIP_ESC;
                    escaped = false;
                } else {
                    *output = SLIP_ESC_ESC;
                }
                return true;
            case SLIP_END:
                if (escaped) {
                    return false;
                }
                packet_complete_flag = true;
                return false;
            default:
                if (escaped) {
                    return false;
                }
                *output = byte;
                return true;
        }
    }

    [[nodiscard]] constexpr bool packet_complete() const {
        return packet_complete_flag;
    }

    constexpr void reset() {
        escaped = false;
        packet_complete_flag = false;
    }

};


#endif //SLIP_HPP
