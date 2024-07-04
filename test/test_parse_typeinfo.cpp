#include <vector>

#include "type_info.hpp"

#include "catch2/catch_test_macros.hpp"
#include "catch2/generators/catch_generators.hpp"

namespace Catch {
    template<>
    struct StringMaker<TypeInfo> {
        static std::string convert( TypeInfo const& value ) {
            return value.to_string();
        }
    };
}
struct TypeInfoTestData {
    std::string token;
    TypeInfo expected;
};

TEST_CASE("Expression evaluator", "") {
    // SECTION("Simple expressions") {
    //     REQUIRE(evaluate_expression("1") == 1);
    //     REQUIRE(evaluate_expression("2") == 2);
    //     REQUIRE(evaluate_expression("1 + 1") == 2);
    // }

    SECTION("Operator precedence") {
        REQUIRE(ExpressionEvaluator("1+2*3").evaluate() == 7);
        REQUIRE(ExpressionEvaluator("1+2/3").evaluate() == 1);
        REQUIRE(ExpressionEvaluator("1-2+3").evaluate() == 2);
        REQUIRE(ExpressionEvaluator("1-2*3").evaluate() == 1);
    }

    SECTION("Parentheses") {
        REQUIRE(ExpressionEvaluator("(1+2)*3").evaluate() == 9);
        REQUIRE(ExpressionEvaluator("1+(2*3)").evaluate() == 7);
        REQUIRE(ExpressionEvaluator("(1+2)/3").evaluate() == 1);
        REQUIRE(ExpressionEvaluator("1-(2+3)").evaluate() == 0);
    }

    SECTION("Invalid expressions") {
        REQUIRE_THROWS(ExpressionEvaluator("1+2"));
        REQUIRE_THROWS(ExpressionEvaluator("a"));
        REQUIRE_THROWS(ExpressionEvaluator("[]"));
        REQUIRE_THROWS(ExpressionEvaluator("(1+2"));
        REQUIRE_THROWS(ExpressionEvaluator("1 + 2"));
        REQUIRE_THROWS(ExpressionEvaluator("1.2"));
    }
}

TEST_CASE("Parse typeinfo", "[parse_typeinfo]") {

    SECTION("Scalar types") {
        REQUIRE(parse_type_info("u8") == TypeInfo::create_scalar(PrimitiveType::u8));
        REQUIRE(parse_type_info("u16") == TypeInfo::create_scalar(PrimitiveType::u16));
        REQUIRE(parse_type_info("u32") == TypeInfo::create_scalar(PrimitiveType::u32));
        REQUIRE(parse_type_info("u64") == TypeInfo::create_scalar(PrimitiveType::u64));
        REQUIRE(parse_type_info("i8") == TypeInfo::create_scalar(PrimitiveType::i8));
        REQUIRE(parse_type_info("i16") == TypeInfo::create_scalar(PrimitiveType::i16));
        REQUIRE(parse_type_info("i32") == TypeInfo::create_scalar(PrimitiveType::i32));
        REQUIRE(parse_type_info("i64") == TypeInfo::create_scalar(PrimitiveType::i64));
        REQUIRE(parse_type_info("f32") == TypeInfo::create_scalar(PrimitiveType::f32));
        REQUIRE(parse_type_info("f64") == TypeInfo::create_scalar(PrimitiveType::f64));
    }

    SECTION("Variable array types") {
        REQUIRE(parse_type_info("u8[]") == TypeInfo::create_variable_array(PrimitiveType::u8));
        REQUIRE(parse_type_info("u16[]") == TypeInfo::create_variable_array(PrimitiveType::u16));
        REQUIRE(parse_type_info("u32[]") == TypeInfo::create_variable_array(PrimitiveType::u32));
        REQUIRE(parse_type_info("u64[]") == TypeInfo::create_variable_array(PrimitiveType::u64));
        REQUIRE(parse_type_info("i8[]") == TypeInfo::create_variable_array(PrimitiveType::i8));
        REQUIRE(parse_type_info("i16[]") == TypeInfo::create_variable_array(PrimitiveType::i16));
        REQUIRE(parse_type_info("i32[]") == TypeInfo::create_variable_array(PrimitiveType::i32));
        REQUIRE(parse_type_info("i64[]") == TypeInfo::create_variable_array(PrimitiveType::i64));
        REQUIRE(parse_type_info("f32[]") == TypeInfo::create_variable_array(PrimitiveType::f32));
        REQUIRE(parse_type_info("f64[]") == TypeInfo::create_variable_array(PrimitiveType::f64));
    }

    SECTION("Fixed array types") {
        REQUIRE(parse_type_info("u8[1]") == TypeInfo::create_fixed_array(PrimitiveType::u8, 1));
        REQUIRE(parse_type_info("u16[2]") == TypeInfo::create_fixed_array(PrimitiveType::u16, 2));
        REQUIRE(parse_type_info("u32[3]") == TypeInfo::create_fixed_array(PrimitiveType::u32, 3));
        REQUIRE(parse_type_info("u64[4]") == TypeInfo::create_fixed_array(PrimitiveType::u64, 4));
        REQUIRE(parse_type_info("i8[5]") == TypeInfo::create_fixed_array(PrimitiveType::i8, 5));
        REQUIRE(parse_type_info("i16[6]") == TypeInfo::create_fixed_array(PrimitiveType::i16, 6));
        REQUIRE(parse_type_info("i32[7]") == TypeInfo::create_fixed_array(PrimitiveType::i32, 7));
        REQUIRE(parse_type_info("i64[8]") == TypeInfo::create_fixed_array(PrimitiveType::i64, 8));
        REQUIRE(parse_type_info("f32[9]") == TypeInfo::create_fixed_array(PrimitiveType::f32, 9));
        REQUIRE(parse_type_info("f64[10]") == TypeInfo::create_fixed_array(PrimitiveType::f64, 10));
    }

    SECTION("Fixed array types with expressions") {
        SECTION("Simple expressions") {
            REQUIRE(parse_type_info("u8[1+1]") == TypeInfo::create_fixed_array(PrimitiveType::u8, 2));
            REQUIRE(parse_type_info("u8[2*2]") == TypeInfo::create_fixed_array(PrimitiveType::u8, 4));
            REQUIRE(parse_type_info("u8[3/3]") == TypeInfo::create_fixed_array(PrimitiveType::u8, 1));
            REQUIRE(parse_type_info("u8[4-1]") == TypeInfo::create_fixed_array(PrimitiveType::u8, 3));
        }

        SECTION("Operator precedence") {
            REQUIRE(parse_type_info("u8[1+2*3]") == TypeInfo::create_fixed_array(PrimitiveType::u8, 7));
            REQUIRE(parse_type_info("u8[1+2/3]") == TypeInfo::create_fixed_array(PrimitiveType::u8, 1));
            REQUIRE(parse_type_info("u8[1-2+3]") == TypeInfo::create_fixed_array(PrimitiveType::u8, 2));
            REQUIRE(parse_type_info("u8[1-2*3]") == TypeInfo::create_fixed_array(PrimitiveType::u8, 1));
        }

        SECTION("Parentheses") {
            REQUIRE(parse_type_info("u8[(1+2)*3]") == TypeInfo::create_fixed_array(PrimitiveType::u8, 9));
            REQUIRE(parse_type_info("u8[1+(2*3)]") == TypeInfo::create_fixed_array(PrimitiveType::u8, 7));
            REQUIRE(parse_type_info("u8[(1+2)/3]") == TypeInfo::create_fixed_array(PrimitiveType::u8, 1));
            REQUIRE(parse_type_info("u8[1-(2+3)]") == TypeInfo::create_fixed_array(PrimitiveType::u8, 0));
        }

        SECTION("Invalid expressions") {
            REQUIRE_THROWS(parse_type_info("u8[1+2"));
            REQUIRE_THROWS(parse_type_info("u8[a]"));
            REQUIRE_THROWS(parse_type_info("u8[[]"));
            REQUIRE_THROWS(parse_type_info("u8[(1+2]"));
            REQUIRE_THROWS(parse_type_info("u8[1 + 2]"));

        }
    }

    SECTION("Invalid types") {
        REQUIRE_THROWS(parse_type_info("int"));
        REQUIRE_THROWS(parse_type_info(""));
        REQUIRE_THROWS(parse_type_info(" "));
        REQUIRE_THROWS(parse_type_info("[]"));
        REQUIRE_THROWS(parse_type_info("[1]"));
        REQUIRE_THROWS(parse_type_info("[0]"));
    }

}