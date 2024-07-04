//
// Created by Obi Davis on 03/07/2024.
//

#ifndef SHA_HPP
#define SHA_HPP

#include <array>
#include <zpp_bits.h>

constexpr auto sha256(std::span<const std::byte> original_message)
{
    using zpp::bits::numbers::big_endian;
    auto rotate_right = [](auto n, auto c) {
        return (n >> c) | (n << ((sizeof(n) * CHAR_BIT) - c));
    };
    auto align = [](auto v, auto a) { return (v + (a - 1)) / a * a; };

    auto h0 = big_endian{std::uint32_t{0x6a09e667u}};
    auto h1 = big_endian{std::uint32_t{0xbb67ae85u}};
    auto h2 = big_endian{std::uint32_t{0x3c6ef372u}};
    auto h3 = big_endian{std::uint32_t{0xa54ff53au}};
    auto h4 = big_endian{std::uint32_t{0x510e527fu}};
    auto h5 = big_endian{std::uint32_t{0x9b05688cu}};
    auto h6 = big_endian{std::uint32_t{0x1f83d9abu}};
    auto h7 = big_endian{std::uint32_t{0x5be0cd19u}};

    std::array k{big_endian{std::uint32_t{0x428a2f98u}}, big_endian{std::uint32_t{0x71374491u}},
                 big_endian{std::uint32_t{0xb5c0fbcfu}}, big_endian{std::uint32_t{0xe9b5dba5u}},
                 big_endian{std::uint32_t{0x3956c25bu}}, big_endian{std::uint32_t{0x59f111f1u}},
                 big_endian{std::uint32_t{0x923f82a4u}}, big_endian{std::uint32_t{0xab1c5ed5u}},
                 big_endian{std::uint32_t{0xd807aa98u}}, big_endian{std::uint32_t{0x12835b01u}},
                 big_endian{std::uint32_t{0x243185beu}}, big_endian{std::uint32_t{0x550c7dc3u}},
                 big_endian{std::uint32_t{0x72be5d74u}}, big_endian{std::uint32_t{0x80deb1feu}},
                 big_endian{std::uint32_t{0x9bdc06a7u}}, big_endian{std::uint32_t{0xc19bf174u}},
                 big_endian{std::uint32_t{0xe49b69c1u}}, big_endian{std::uint32_t{0xefbe4786u}},
                 big_endian{std::uint32_t{0x0fc19dc6u}}, big_endian{std::uint32_t{0x240ca1ccu}},
                 big_endian{std::uint32_t{0x2de92c6fu}}, big_endian{std::uint32_t{0x4a7484aau}},
                 big_endian{std::uint32_t{0x5cb0a9dcu}}, big_endian{std::uint32_t{0x76f988dau}},
                 big_endian{std::uint32_t{0x983e5152u}}, big_endian{std::uint32_t{0xa831c66du}},
                 big_endian{std::uint32_t{0xb00327c8u}}, big_endian{std::uint32_t{0xbf597fc7u}},
                 big_endian{std::uint32_t{0xc6e00bf3u}}, big_endian{std::uint32_t{0xd5a79147u}},
                 big_endian{std::uint32_t{0x06ca6351u}}, big_endian{std::uint32_t{0x14292967u}},
                 big_endian{std::uint32_t{0x27b70a85u}}, big_endian{std::uint32_t{0x2e1b2138u}},
                 big_endian{std::uint32_t{0x4d2c6dfcu}}, big_endian{std::uint32_t{0x53380d13u}},
                 big_endian{std::uint32_t{0x650a7354u}}, big_endian{std::uint32_t{0x766a0abbu}},
                 big_endian{std::uint32_t{0x81c2c92eu}}, big_endian{std::uint32_t{0x92722c85u}},
                 big_endian{std::uint32_t{0xa2bfe8a1u}}, big_endian{std::uint32_t{0xa81a664bu}},
                 big_endian{std::uint32_t{0xc24b8b70u}}, big_endian{std::uint32_t{0xc76c51a3u}},
                 big_endian{std::uint32_t{0xd192e819u}}, big_endian{std::uint32_t{0xd6990624u}},
                 big_endian{std::uint32_t{0xf40e3585u}}, big_endian{std::uint32_t{0x106aa070u}},
                 big_endian{std::uint32_t{0x19a4c116u}}, big_endian{std::uint32_t{0x1e376c08u}},
                 big_endian{std::uint32_t{0x2748774cu}}, big_endian{std::uint32_t{0x34b0bcb5u}},
                 big_endian{std::uint32_t{0x391c0cb3u}}, big_endian{std::uint32_t{0x4ed8aa4au}},
                 big_endian{std::uint32_t{0x5b9cca4fu}}, big_endian{std::uint32_t{0x682e6ff3u}},
                 big_endian{std::uint32_t{0x748f82eeu}}, big_endian{std::uint32_t{0x78a5636fu}},
                 big_endian{std::uint32_t{0x84c87814u}}, big_endian{std::uint32_t{0x8cc70208u}},
                 big_endian{std::uint32_t{0x90befffau}}, big_endian{std::uint32_t{0xa4506cebu}},
                 big_endian{std::uint32_t{0xbef9a3f7u}}, big_endian{std::uint32_t{0xc67178f2u}}};

    // constexpr auto original_message = to_bytes<Object>();
    constexpr auto chunk_size = 512 / CHAR_BIT;
    constexpr auto message = to_bytes<
        original_message,
        std::byte{0x80},
        std::array<std::byte,
                   align(original_message.size() + sizeof(std::byte{0x80}),
                         chunk_size) -
                       original_message.size() - sizeof(std::byte{0x80}) -
                       sizeof(std::uint64_t{original_message.size()})>{},
        big_endian<std::uint64_t>{original_message.size() * CHAR_BIT}>();

    for (auto chunk :
         from_bytes<message,
                    std::array<std::array<big_endian<std::uint32_t>, 16>,
                               message.size() / chunk_size>>()) {
        std::array<big_endian<std::uint32_t>, 64> w;
        std::copy(std::begin(chunk), std::end(chunk), std::begin(w));

        for (std::size_t i = 16; i < w.size(); ++i) {
            auto s0 = rotate_right(w[i - 15], 7) ^
                      rotate_right(w[i - 15], 18) ^ (w[i - 15] >> 3);
            auto s1 = rotate_right(w[i - 2], 17) ^
                      rotate_right(w[i - 2], 19) ^ (w[i - 2] >> 10);
            w[i] = w[i - 16] + s0 + w[i - 7] + s1;
        }

        auto a = h0;
        auto b = h1;
        auto c = h2;
        auto d = h3;
        auto e = h4;
        auto f = h5;
        auto g = h6;
        auto h = h7;

        for (std::size_t i = 0; i < w.size(); ++i) {
            auto s1 = rotate_right(e, 6) ^ rotate_right(e, 11) ^
                      rotate_right(e, 25);
            auto ch = (e & f) ^ ((~e) & g);
            auto temp1 = h + s1 + ch + k[i] + w[i];
            auto s0 = rotate_right(a, 2) ^ rotate_right(a, 13) ^
                      rotate_right(a, 22);
            auto maj = (a & b) ^ (a & c) ^ (b & c);
            auto temp2 = s0 + maj;

            h = g;
            g = f;
            f = e;
            e = d + temp1;
            d = c;
            c = b;
            b = a;
            a = temp1 + temp2;
        }

        h0 = h0 + a;
        h1 = h1 + b;
        h2 = h2 + c;
        h3 = h3 + d;
        h4 = h4 + e;
        h5 = h5 + f;
        h6 = h6 + g;
        h7 = h7 + h;
    }

    std::array<std::byte, 32> digest_data;
    out{digest_data}(h0, h1, h2, h3, h4, h5, h6, h7).or_throw();

    Digest digest;
    in{digest_data}(digest).or_throw();
    return digest;
}

#endif //SHA_HPP
