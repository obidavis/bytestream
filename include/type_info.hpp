//
// Created by Obi Davis on 12/03/2024.
//

#ifndef TYPE_INFO_HPP
#define TYPE_INFO_HPP

#include <cstdint>
#include <variant>
#include <optional>
#include <regex>
#include <string>
#include <ranges>
#include "ext.h"
#include "zpp_bits.h"
#include "magic_enum.hpp"
#include "atom_views.hpp"

struct type_info {
    type_info(const std::string &type_string);
    enum class primitive_type {
        u8, i8, u16, i16, u32, i32, u64, i64, f32, f64 // TODO: Add string and boolean types
    } type;
    size_t size;
    static constexpr size_t variable_size = -1;
    [[nodiscard]] bool is_variable_length() const { return size == variable_size; }
    [[nodiscard]] bool is_scalar() const { return size == 1; }
    [[nodiscard]] std::string to_string() const;
    [[nodiscard]] size_t size_bytes() const {
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
};

inline type_info::type_info(const std::string & type_string) {
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

inline std::string type_info::to_string() const {
    std::string result = std::string(magic_enum::enum_name(type));
    if (!is_scalar()) {
        if (is_variable_length()) {
            result += "[]";
        } else {
            result += "[" + std::to_string(size) + "]";
        }
    }
    return result;
}

template <typename T>
struct storage_base {
    std::vector<T> data;
    type_info info;
    storage_base(const type_info &info) : info(info) {
        if (!info.is_variable_length()) {
            data.resize(info.size);
        }
    }

    auto begin() { return data.begin(); }
    auto end() { return data.end(); }
    auto begin() const { return data.begin(); }
    auto end() const { return data.end(); }

    template <typename U>
    requires std::is_arithmetic_v<U>
    void load(U value) {
        if (info.is_variable_length()) {
            data.resize(1);
        }
        data[0] = value;
    }

    void load(std::span<const t_atom> values) {
        constexpr static auto from_atom = [](const t_atom &atom) {
            if constexpr (std::is_integral_v<T>) {
                return static_cast<T>(atom_getlong(&atom));
            } else if constexpr (std::is_floating_point_v<T>) {
                return static_cast<T>(atom_getfloat(&atom));
            } else {
                static_assert(sizeof(T) == 0, "Unsupported type");
            }
        };

        if (info.is_variable_length()) {
            data.resize(values.size());
            std::transform(values.begin(), values.end(), data.begin(), from_atom);
        } else {
            auto end = values.begin() + std::min(values.size(), data.size());
            std::transform(values.begin(), end, data.begin(), from_atom);
        }
    }

    constexpr static auto serialize(auto &archive, storage_base &self) {
        if (self.info.is_variable_length()) {
            return archive(self.data);
        } else {
            return archive(zpp::bits::unsized(self.data));
        }
    }
};

using storage_variant = std::variant<
    storage_base<uint8_t>,
    storage_base<uint16_t>,
    storage_base<uint32_t>,
    storage_base<uint64_t>,
    storage_base<int8_t>,
    storage_base<int16_t>,
    storage_base<int32_t>,
    storage_base<int64_t>,
    storage_base<float>,
    storage_base<double>
>;

class storage {
public:
    storage(const type_info &info) : sv(create_variant(info)) {}

    template <typename T>
    void load(T value) {
        std::visit([&value](auto &s) { s.load(value); }, sv);
    }

    type_info info() {
        return std::visit([](auto &s) { return s.info; }, sv);
    }

    [[nodiscard]] size_t size() const {
        return std::visit([](auto &s) { return s.data.size(); }, sv);
    }

    static constexpr auto serialize(auto &archive, storage &self) {
        return std::visit([&archive](auto &s) { return archive(s); }, self.sv);
    }

    template <typename OutIt>
    auto store_to_atoms(OutIt out) const {
        return std::visit([out](auto &s) mutable {
            for (const auto &value: s) {
                *out++ = atom_from(value);
            }
            return out;
        }, sv);
    }

private:
    storage_variant sv;

    static storage_variant create_variant(const type_info &info) {
        switch (info.type) {
#define CASE(type, type_enum)                               \
            case type_info::primitive_type::type_enum:      \
                return storage_base<type>(info);
            CASE(uint8_t, u8)
            CASE(uint16_t, u16)
            CASE(uint32_t, u32)
            CASE(uint64_t, u64)
            CASE(int8_t, i8)
            CASE(int16_t, i16)
            CASE(int32_t, i32)
            CASE(int64_t, i64)
            CASE(float, f32)
            CASE(double, f64)
#undef CASE
            default:
                throw std::runtime_error("Unsupported type");
        }
    }

};

#endif //TYPE_INFO_HPP
