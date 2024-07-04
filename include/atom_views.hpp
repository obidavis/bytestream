//
// Created by Obi Davis on 01/07/2024.
//

#ifndef ATOM_VIEWS_HPP
#define ATOM_VIEWS_HPP

#include <ext_mess.h>
#include <ext_obex.h>
#include <ranges>

inline std::string to_string(const t_atom &a) {
    if (atom_gettype(&a) == A_SYM) {
        return {atom_getsym(&a)->s_name};
    }
    throw std::invalid_argument("Expected symbol");
}

inline std::span<t_atom> get_non_attr_args(long argc, t_atom *argv) {
    size_t offset = attr_args_offset((short)argc, argv);
    return {argv, offset};
}

inline std::span<t_atom> get_attr_args(long argc, t_atom *argv) {
    size_t offset = attr_args_offset((short)argc, argv);
    return {argv + offset, static_cast<size_t>(argc) - offset};
}

inline std::pair<std::span<t_atom>, std::span<t_atom>> get_args_and_attrs(long argc, t_atom *argv) {
    size_t offset = attr_args_offset((short)argc, argv);
    return {{argv, offset}, {argv + offset, static_cast<size_t>(argc) - offset}};
}

template <typename T>
requires std::is_arithmetic_v<T>
t_atom atom_from(T value) {
    if constexpr (std::is_integral_v<T>) {
        return { .a_type = A_LONG, .a_w.w_long = static_cast<long>(value) };
    } else if constexpr (std::is_floating_point_v<T>) {
        return { .a_type = A_FLOAT, .a_w.w_float = static_cast<double>(value) };
    } else {
        static_assert(sizeof(T) == 0, "Unsupported type");
        __builtin_unreachable();
    }
}






#endif //ATOM_VIEWS_HPP
