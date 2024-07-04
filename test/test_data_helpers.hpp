//
// Created by Obi Davis on 26/04/2024.
//

#ifndef BYTESTREAM_TEST_DATA_HELPERS_HPP
#define BYTESTREAM_TEST_DATA_HELPERS_HPP

#include <vector>
#include <numeric>
#include <cstdint>

template <std::integral Int = int>
constexpr std::vector<Int> vec_from_range(Int start, Int end) {
    std::vector<Int> v(end - start);
    std::iota(v.begin(), v.end(), start);
    return v;
}

template <typename T, typename U>
concept ValueOrVector = std::convertible_to<T, U> || std::same_as<T, std::vector<U>>;

template <std::integral Int = int>
constexpr std::vector<Int> compose_vec(ValueOrVector<Int> auto &&... args) {
    constexpr auto add_to_vec = [](std::vector<Int> &v, const auto &arg) {
        using T = std::remove_cvref_t<decltype(arg)>;
        if constexpr (std::same_as<T, std::vector<Int>>) {
            v.insert(v.end(), arg.begin(), arg.end());
        } else {
            v.push_back(arg);
        }
    };

    std::vector<Int> v;
    (add_to_vec(v, args), ...);
    return v;
}

#endif //BYTESTREAM_TEST_DATA_HELPERS_HPP
