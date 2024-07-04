//
// Created by Obi Davis on 18/03/2024.
//

#ifndef FROM_BYTES_HPP
#define FROM_BYTES_HPP

#include <tuple>
#include <cstring>

template <typename T, typename... Ts>
struct tuple_or_value {
    using type = std::tuple<T, Ts...>;
};
template <typename T>
struct tuple_or_value {
    using type = T;
};

template <typename ...Ts>
using tuple_or_value_t = typename tuple_or_value<Ts...>::type;

template <typename ...Ts>
tuple_or_value_t<Ts...> from_bytes(const char *bytes, size_t size) {
    tuple_or_value_t<Ts...> result;
    std::memcpy(&result, bytes, size);
    return result;
}

#endif //FROM_BYTES_HPP
