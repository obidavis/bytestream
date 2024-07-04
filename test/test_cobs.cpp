#include <vector>

#include "catch2/catch_test_macros.hpp"
#include "catch2/generators/catch_generators.hpp"

#include "COBS.hpp"
#include "test_data_helpers.hpp"

/*
 * From https://en.wikipedia.org/wiki/Consistent_Overhead_Byte_Stuffing#Encoding_examples
 *
 * Test  	Unencoded data (hex) 	Encoded with COBS (hex)
 * 1 	    00 	                    01 01 00
 * 2 	    00 00 	                01 01 01 00
 * 3 	    00 11 00 	            01 02 11 01 00
 * 4 	    11 22 00 33 	        03 11 22 02 33 00
 * 5 	    11 22 33 44 	        05 11 22 33 44 00
 * 6 	    11 00 00 00 	        02 11 01 01 01 00
 * 7 	    01 02 03 ... FD FE 	    FF 01 02 03 ... FD FE 00
 * 8 	    00 01 02 ... FC FD FE 	01 FF 01 02 ... FC FD FE 00
 * 9 	    01 02 03 ... FD FE FF 	FF 01 02 03 ... FD FE 02 FF 00
 * 10 	    02 03 04 ... FE FF 00 	FF 02 03 04 ... FE FF 01 01 00
 * 11 	    03 04 05 ... FF 00 01 	FE 03 04 05 ... FF 02 01 00
 *
 */

struct COBSData {
    std::vector<int> decoded;
    std::vector<int> encoded;
};

TEST_CASE("COBS encoding and decoding", "[cobs]") {
    auto data = GENERATE(values<COBSData>({
        {
            {0x00},
            {0x01, 0x01, 0x00}
        },
        {
            {0x00, 0x00},
            {0x01, 0x01, 0x01, 0x00}
        },
        {
            {0x00, 0x11, 0x00},
            {0x01, 0x02, 0x11, 0x01, 0x00}
        },
        {
            {0x11, 0x22, 0x00, 0x33},
            {0x03, 0x11, 0x22, 0x02, 0x33, 0x00}
        },
        {
            {0x11, 0x22, 0x33, 0x44},
            {0x05, 0x11, 0x22, 0x33, 0x44, 0x00}
        },
        {
            {0x11, 0x00, 0x00, 0x00},
            {0x02, 0x11, 0x01, 0x01, 0x01, 0x00}
        },
        {
            vec_from_range(0x01, 0xFF),
            compose_vec(0xFF, vec_from_range(0x01, 0xFF), 0x00)
        },
        {
            vec_from_range(0x00, 0xFF),
            compose_vec(0x01, 0xFF, vec_from_range(0x01, 0xFF), 0x00)
        },
        {
            vec_from_range(0x01, 0x100),
            compose_vec(0xFF, vec_from_range(0x01, 0xFF), 0x02, 0xFF, 0x00)
        },
        {
            compose_vec(vec_from_range(0x02, 0x100), 0x00),
            compose_vec(0xFF, vec_from_range(0x02, 0x100), 0x01, 0x01, 0x00)
        },
        {
            compose_vec(vec_from_range(0x03, 0x100), 0x00, 0x01),
            compose_vec(0xFE, vec_from_range(0x03, 0x100), 0x02, 0x01, 0x00)
        }
    }));

    SECTION("Max encoded length") {
        const size_t max_length = cobs_encoded_max_length(data.decoded.size());
        REQUIRE(max_length >= data.encoded.size());
    }

    SECTION("Encoding") {
        std::vector<int> encoded(cobs_encoded_max_length(data.decoded.size()));
        auto encoded_end = cobs_encode_frame(data.decoded.begin(), data.decoded.end(), encoded.begin());
        encoded.resize(std::distance(encoded.begin(), encoded_end));
        REQUIRE(encoded == data.encoded);
    }

    SECTION("Decoding") {
        std::vector<int> decoded(data.decoded.size());
        auto decoded_end = cobs_decode_frame(data.encoded.begin(), data.encoded.end(), decoded.begin());
        decoded.resize(std::distance(decoded.begin(), decoded_end));
        REQUIRE(decoded == data.decoded);
    }

    SECTION("Piecemeal decoding") {
        COBSDecoder decoder;
        std::vector<int> decoded;
        decoded.reserve(data.encoded.size());

        for (const int byte: data.encoded) {
            if (decoder.packet_complete()) {
                break;
            }

            int decoded_byte;
            if (decoder.process_byte(byte, &decoded_byte)) {
                decoded.push_back(decoded_byte);
            }
        }

        REQUIRE(decoder.packet_complete());
        REQUIRE(decoded == data.decoded);
    }
}
