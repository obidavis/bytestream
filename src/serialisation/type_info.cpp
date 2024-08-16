//
// Created by Obi Davis on 31/07/2024.
//

#include "type_info.hpp"
#include <regex>
#include "magic_enum.hpp"

type_info::type_info(const std::string & type_string) {
    static const std::regex re(R"((\w+)(\[(\d+)?\])?)");
    std::smatch matches;
    if (!std::regex_match(type_string, matches, re)) {
        throw std::runtime_error("Invalid type string: " + type_string);
    }
    if (auto type_opt = magic_enum::enum_cast<primitive_type>(matches[1].str())) {
        type = *type_opt;
        if (matches[2].matched) {
            if (matches[3].str().empty()) {
                size = variable_size;
            } else {
                size = std::stoull(matches[3].str());
                if (size == 0) {
                    throw std::runtime_error("Array array_length must be greater than zero");
                }
            }
        } else {
            size = 1;
        }
    } else {
        throw std::runtime_error("Invalid primitive type: " + matches[1].str());
    }
}

std::string type_info::to_string() const {
    auto result = std::string(magic_enum::enum_name(type));
    if (!is_scalar()) {
        if (is_variable_length()) {
            result += "[]";
        } else {
            result += "[" + std::to_string(size) + "]";
        }
    }
    return result;
}

size_t type_info::size_bytes() const {
    switch (type) {
        case primitive_type::u8:
        case primitive_type::i8:
            return size;
        case primitive_type::u16:
        case primitive_type::i16:
            return size * 2;
        case primitive_type::u32:
        case primitive_type::i32:
        case primitive_type::f32:
            return size * 4;
        case primitive_type::u64:
        case primitive_type::i64:
        case primitive_type::f64:
            return size * 8;
        default:
            throw std::runtime_error("Invalid type");
    }
}
