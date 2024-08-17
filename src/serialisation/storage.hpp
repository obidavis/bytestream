//
// Created by Obi Davis on 31/07/2024.
//

#ifndef STORAGE_HPP
#define STORAGE_HPP

#include "type_info.hpp"
#include <vector>
#include "maxutils/jit_matrix_view_v2.hpp"

template <typename T>
struct atom_storage {
    std::vector<T> data;
    type_info info;
    atom_storage(const type_info &info) : info(info) {
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

    void load(t_jit_object *matrix) {
        t_jit_matrix_info matrix_info;
        jit_object_method(matrix, _jit_sym_getinfo, &matrix_info);
        if (matrix_info.dimcount > 2) {
            throw std::runtime_error("more than 2d matrices not supported");
        }


        auto copy = [this]<typename U>(maxutils::matrix_view<U> m) {
            if (info.is_variable_length()) {
                data.resize(m.nrows() * m.ncols());
            }

            for (size_t row_index = 0; row_index < m.nrows(); ++row_index) {
                std::span row_span = m.row(row_index).as_1d_span();
                auto data_start = data.begin() + row_index * row_span.size();
                std::copy(row_span.begin(), row_span.end(), data_start);
            }
        };

        if (matrix_info.type == _jit_sym_char) {
            copy(maxutils::matrix_view<char>(matrix));
        } else if (matrix_info.type == _jit_sym_long) {
            copy(maxutils::matrix_view<int32_t>(matrix));
        } else if (matrix_info.type == _jit_sym_float32) {
            copy(maxutils::matrix_view<float>(matrix));
        } else if (matrix_info.type == _jit_sym_float64) {
            copy(maxutils::matrix_view<double>(matrix));
        }
    }

    constexpr static auto serialize(auto &archive, atom_storage &self) {
        if (self.info.is_variable_length()) {
            return archive(self.data);
        } else {
            return archive(zpp::bits::unsized(self.data));
        }
    }
};

using storage_variant = std::variant<
    atom_storage<uint8_t>,
    atom_storage<uint16_t>,
    atom_storage<uint32_t>,
    atom_storage<uint64_t>,
    atom_storage<int8_t>,
    atom_storage<int16_t>,
    atom_storage<int32_t>,
    atom_storage<int64_t>,
    atom_storage<float>,
    atom_storage<double>
>;

struct matrix_storage {

};

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
                return atom_storage<type>(info);
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

#endif //STORAGE_HPP
