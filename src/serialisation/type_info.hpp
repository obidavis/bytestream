//
// Created by Obi Davis on 31/07/2024.
//

#ifndef TYPE_INFO_HPP
#define TYPE_INFO_HPP

#include <string>

struct type_info {
    explicit type_info(const std::string &type_string);
    enum class primitive_type {
        u8, i8, u16, i16, u32, i32, u64, i64, f32, f64 // TODO: Add string and boolean types
    } type;
    size_t size;
    static constexpr size_t variable_size = -1;
    [[nodiscard]] bool is_variable_length() const { return size == variable_size; }
    [[nodiscard]] bool is_scalar() const { return size == 1; }
    [[nodiscard]] std::string to_string() const;
    [[nodiscard]] size_t size_bytes() const;
};

#endif //TYPE_INFO_HPP
