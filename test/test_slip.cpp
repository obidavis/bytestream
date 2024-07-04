//
// Created by Obi Davis on 26/04/2024.
//

#include <vector>

#include "catch2/catch_test_macros.hpp"
#include "catch2/generators/catch_generators_range.hpp"

#include "SLIP.hpp"
#include "test_data_helpers.hpp"

struct SLIPData {
    std::vector<int> decoded;
    std::vector<int> encoded;
};

TEST_CASE("SLIP encoding and decoding", "[slip]") {
    auto data = GENERATE(values<SLIPData>({
        {
            {0x00},
            {0x00, SLIP_END}
        },
        {
            {0x00, 0x00},
            {0x00, 0x00, SLIP_END}
        },
        {
            {0x00, 0x11, 0x22},
            {0x00, 0x11, 0x22, SLIP_END}
        },
        {
            {SLIP_END},
            {SLIP_ESC, SLIP_ESC_END, SLIP_END}
        },
        {
            {SLIP_ESC},
            {SLIP_ESC, SLIP_ESC_ESC, SLIP_END}
        },
        {
            {SLIP_ESC_END},
            {SLIP_ESC_END, SLIP_END}
        },
        {
            {SLIP_ESC_ESC},
            {SLIP_ESC_ESC, SLIP_END}
        },
        {
            {SLIP_END, SLIP_ESC},
            {SLIP_ESC, SLIP_ESC_END, SLIP_ESC, SLIP_ESC_ESC, SLIP_END}
        },
        {
            {SLIP_ESC, SLIP_END},
            {SLIP_ESC, SLIP_ESC_ESC, SLIP_ESC, SLIP_ESC_END, SLIP_END}
        },
        {
            {SLIP_END, SLIP_ESC, SLIP_END},
            {SLIP_ESC, SLIP_ESC_END, SLIP_ESC, SLIP_ESC_ESC, SLIP_ESC, SLIP_ESC_END, SLIP_END}
        },
        {
            {SLIP_ESC, SLIP_END, SLIP_ESC},
            {SLIP_ESC, SLIP_ESC_ESC, SLIP_ESC, SLIP_ESC_END, SLIP_ESC, SLIP_ESC_ESC, SLIP_END}
        },
        {
            vec_from_range(0x00, 0x100),
            compose_vec(vec_from_range(0x00, (int)SLIP_END),
                        SLIP_ESC, SLIP_ESC_END,
                        vec_from_range(SLIP_END + 1, (int)SLIP_ESC),
                        SLIP_ESC, SLIP_ESC_ESC,
                        vec_from_range(SLIP_ESC + 1, 0x100),
                        SLIP_END)
        }
    }));

    SECTION("Max encoded length") {
        const size_t max_length = slip_encoded_max_length(data.decoded.size());
        REQUIRE(max_length >= data.encoded.size());
    }

    SECTION("Encoding") {
        std::vector<int> encoded;
        encoded.resize(slip_encoded_max_length(data.decoded.size()));
        auto encoded_end = slip_encode_frame(data.decoded.begin(), data.decoded.end(), encoded.begin());
        encoded.resize(std::distance(encoded.begin(), encoded_end));
        REQUIRE(encoded == data.encoded);
    }

    SECTION("Decoding") {
        std::vector<int> decoded;
        decoded.resize(data.encoded.size());
        auto decoded_end = slip_decode_frame(data.encoded.begin(), data.encoded.end(), decoded.begin());
        decoded.resize(std::distance(decoded.begin(), decoded_end));
        REQUIRE(decoded == data.decoded);
    }

    SECTION("Piecemeal decoding") {
        SLIPDecoder decoder;
        std::vector<int> decoded;

        for (const int byte: data.encoded) {
            int decoded_byte;
            if (decoder.process_byte(byte, &decoded_byte)) {
                decoded.push_back(decoded_byte);
            }
            if (decoder.packet_complete()) {
                break;
            }
        }

        REQUIRE(decoder.packet_complete());
        REQUIRE(decoded == data.decoded);
    }
}