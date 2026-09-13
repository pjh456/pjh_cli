#include <doctest/doctest.h>

#include <filesystem>
#include <iostream>
#include <pjh_cli/core/converter.hpp>
#include <string>
#include <string_view>
#include <variant>

namespace
{
    struct Widget
    {
    };
}  // namespace

/// @brief Custom Converter<T> used to verify the free-form string factory path.
template <>
struct pjh::cli::Converter<Widget>
{
    static auto from_string(std::string_view s, std::string_view display = {})
        -> pjh::cli::CliResult<Widget>
    {
        if (s == "ok")
            return pjh::cli::CliResult<Widget>::Ok(Widget{});
        return pjh::cli::CliResult<Widget>::Err(
            pjh::cli::ErrorFactory::type_conversion_error(display, s, "widget"));
    }
};

TEST_CASE("Converter int")
{
    using pjh::cli::Converter;
    auto r1 = Converter<int>::from_string("42");
    CHECK(r1.is_ok());
    CHECK(r1.unwrap() == 42);

    auto r2 = Converter<int>::from_string("-10");
    CHECK(r2.is_ok());
    CHECK(r2.unwrap() == -10);

    auto r3 = Converter<int>::from_string("abc");
    CHECK(r3.is_err());
}

TEST_CASE("Converter long")
{
    using pjh::cli::Converter;
    auto r4 = Converter<long>::from_string("2147483647");
    CHECK(r4.is_ok());
    CHECK(r4.unwrap() == 2147483647L);
}

TEST_CASE("Converter unsigned")
{
    using pjh::cli::Converter;
    auto r5 = Converter<unsigned>::from_string("100");
    CHECK(r5.is_ok());
    CHECK(r5.unwrap() == 100u);

    auto r6 = Converter<unsigned>::from_string("-1");
    CHECK(r6.is_err());
}

TEST_CASE("Converter bool")
{
    using pjh::cli::Converter;
    auto r7 = Converter<bool>::from_string("true");
    CHECK(r7.is_ok());
    CHECK(r7.unwrap() == true);

    auto r8 = Converter<bool>::from_string("FALSE");
    CHECK(r8.is_ok());
    CHECK(r8.unwrap() == false);

    auto r9 = Converter<bool>::from_string("1");
    CHECK(r9.is_ok());
    CHECK(r9.unwrap() == true);

    auto r10 = Converter<bool>::from_string("yes");
    CHECK(r10.is_ok());
    CHECK(r10.unwrap() == true);

    auto r11 = Converter<bool>::from_string("n");
    CHECK(r11.is_ok());
    CHECK(r11.unwrap() == false);

    auto r12 = Converter<bool>::from_string("bad");
    CHECK(r12.is_err());
}

TEST_CASE("Converter string")
{
    using pjh::cli::Converter;
    auto r13 = Converter<std::string>::from_string("hello world");
    CHECK(r13.is_ok());
    CHECK(r13.unwrap() == "hello world");
}

TEST_CASE("Converter float")
{
    using pjh::cli::Converter;
    auto r14 = Converter<float>::from_string("3.14");
    CHECK(r14.is_ok());
    auto r14_val = r14.unwrap();
    CHECK(r14_val > 3.13f);
    CHECK(r14_val < 3.15f);

    auto r15 = Converter<float>::from_string("not_a_number");
    CHECK(r15.is_err());
}

TEST_CASE("Converter double")
{
    using pjh::cli::Converter;
    auto r16 = Converter<double>::from_string("2.71828");
    CHECK(r16.is_ok());
    auto r16_val = r16.unwrap();
    CHECK(r16_val > 2.71);
    CHECK(r16_val < 2.72);
}

TEST_CASE("Converter long long")
{
    using pjh::cli::Converter;
    auto r17 = Converter<long long>::from_string("9999999999999");
    CHECK(r17.is_ok());
    CHECK(r17.unwrap() == 9999999999999LL);
}

TEST_CASE("Converter unsigned long")
{
    using pjh::cli::Converter;
    auto r18 = Converter<unsigned long>::from_string("3000000000");
    CHECK(r18.is_ok());
}

TEST_CASE("Converter unsigned long long")
{
    using pjh::cli::Converter;
    auto r19 = Converter<unsigned long long>::from_string("18446744073709551615");
    CHECK(r19.is_ok());
}

TEST_CASE("Converter path returns path")
{
    using pjh::cli::Converter;
    auto r = Converter<std::filesystem::path>::from_string("/tmp/x");
    REQUIRE(r.is_ok());
    CHECK(r.unwrap() == std::filesystem::path("/tmp/x"));
}

TEST_CASE("Converter string accepts display argument")
{
    using pjh::cli::Converter;
    auto r = Converter<std::string>::from_string("hi", "--name");
    REQUIRE(r.is_ok());
    CHECK(r.unwrap() == "hi");
}

TEST_CASE("Converter int error carries display and structured type")
{
    using pjh::cli::Converter;
    using pjh::cli::TypeConversionError;
    auto r = Converter<int>::from_string("abc", "--port");
    REQUIRE(r.is_err());
    auto err = r.unwrap_err();
    const auto *info = std::get_if<TypeConversionError>(&err.info());
    REQUIRE(info != nullptr);
    CHECK(info->option_display == "--port");
    CHECK(info->raw_value == "abc");
    CHECK(info->expected_type == "integer");
    CHECK(
        std::string_view(err.what()) ==
        "Parse Error: invalid value 'abc' for '--port': expected integer");
}

TEST_CASE("Converter int accepts leading plus")
{
    using pjh::cli::Converter;
    auto r1 = Converter<int>::from_string("+42");
    REQUIRE(r1.is_ok());
    CHECK(r1.unwrap() == 42);

    auto r2 = Converter<int>::from_string("+0");
    REQUIRE(r2.is_ok());
    CHECK(r2.unwrap() == 0);
}

TEST_CASE("Converter int rejects lone plus and double sign")
{
    using pjh::cli::Converter;
    CHECK(Converter<int>::from_string("+").is_err());
    CHECK(Converter<int>::from_string("+-5").is_err());
    CHECK(Converter<int>::from_string("++5").is_err());
    CHECK(Converter<int>::from_string("+ 5").is_err());
}

TEST_CASE("Converter unsigned accepts leading plus")
{
    using pjh::cli::Converter;
    auto r1 = Converter<unsigned>::from_string("+100");
    REQUIRE(r1.is_ok());
    CHECK(r1.unwrap() == 100u);

    CHECK(Converter<unsigned>::from_string("+-1").is_err());
}

TEST_CASE("Converter float accepts leading plus")
{
    using pjh::cli::Converter;
    auto r1 = Converter<double>::from_string("+3.14");
    REQUIRE(r1.is_ok());
    CHECK(r1.unwrap() == doctest::Approx(3.14));

    auto r2 = Converter<double>::from_string("+.5");
    REQUIRE(r2.is_ok());
    CHECK(r2.unwrap() == doctest::Approx(0.5));

    auto r3 = Converter<double>::from_string("+1e3");
    REQUIRE(r3.is_ok());
    CHECK(r3.unwrap() == doctest::Approx(1000.0));
}

TEST_CASE("Converter double rejects non-finite")
{
    using pjh::cli::Converter;
    CHECK(Converter<double>::from_string("nan").is_err());
    CHECK(Converter<double>::from_string("NaN").is_err());
    CHECK(Converter<double>::from_string("nan(123)").is_err());
    CHECK(Converter<double>::from_string("inf").is_err());
    CHECK(Converter<double>::from_string("-inf").is_err());
    CHECK(Converter<double>::from_string("INF").is_err());
    CHECK(Converter<double>::from_string("infinity").is_err());
    CHECK(Converter<double>::from_string("-Infinity").is_err());
    CHECK(Converter<double>::from_string("+inf").is_err());
}

TEST_CASE("Converter float non-finite error is structured")
{
    using pjh::cli::Converter;
    using pjh::cli::TypeConversionError;
    auto r = Converter<double>::from_string("nan", "--rate");
    REQUIRE(r.is_err());
    auto err = r.unwrap_err();
    const auto *info = std::get_if<TypeConversionError>(&err.info());
    REQUIRE(info != nullptr);
    CHECK(info->option_display == "--rate");
    CHECK(info->raw_value == "nan");
    CHECK(info->expected_type == "float");
    CHECK(
        std::string_view(err.what()) ==
        "Parse Error: invalid value 'nan' for '--rate': expected float");
}

TEST_CASE("Converter int plus error keeps raw value")
{
    using pjh::cli::Converter;
    using pjh::cli::TypeConversionError;
    auto r = Converter<int>::from_string("+abc", "--port");
    REQUIRE(r.is_err());
    auto err = r.unwrap_err();
    const auto *info = std::get_if<TypeConversionError>(&err.info());
    REQUIRE(info != nullptr);
    CHECK(info->raw_value == "+abc");
    CHECK(info->expected_type == "integer");
}

TEST_CASE("detail::expected_type_tag maps builtin converter targets")
{
    using pjh::cli::ExpectedType;
    static_assert(pjh::cli::detail::expected_type_tag<int>() == ExpectedType::Integer);
    static_assert(pjh::cli::detail::expected_type_tag<long>() == ExpectedType::Integer);
    static_assert(
        pjh::cli::detail::expected_type_tag<unsigned>() == ExpectedType::Integer);
    static_assert(
        pjh::cli::detail::expected_type_tag<long long>() == ExpectedType::Integer);
    static_assert(pjh::cli::detail::expected_type_tag<float>() == ExpectedType::Float);
    static_assert(pjh::cli::detail::expected_type_tag<double>() == ExpectedType::Float);
    static_assert(pjh::cli::detail::expected_type_tag<bool>() == ExpectedType::Bool);
    CHECK(true);
}

TEST_CASE("Converter errors carry a stable ExpectedType tag")
{
    using pjh::cli::Converter;
    using pjh::cli::ExpectedType;
    using pjh::cli::TypeConversionError;

    auto int_r = Converter<int>::from_string("abc", "--port");
    REQUIRE(int_r.is_err());
    auto int_err = int_r.unwrap_err();
    const auto *int_info = std::get_if<TypeConversionError>(&int_err.info());
    REQUIRE(int_info != nullptr);
    CHECK(int_info->expected == ExpectedType::Integer);
    CHECK(int_info->expected_type == "integer");
    CHECK(
        std::string_view(int_err.what()) ==
        "Parse Error: invalid value 'abc' for '--port': expected integer");

    auto float_r = Converter<double>::from_string("nan", "--rate");
    REQUIRE(float_r.is_err());
    auto float_err = float_r.unwrap_err();
    const auto *float_info = std::get_if<TypeConversionError>(&float_err.info());
    REQUIRE(float_info != nullptr);
    CHECK(float_info->expected == ExpectedType::Float);
    CHECK(float_info->expected_type == "float");
    CHECK(
        std::string_view(float_err.what()) ==
        "Parse Error: invalid value 'nan' for '--rate': expected float");

    auto bool_r = Converter<bool>::from_string("bad", "--flag");
    REQUIRE(bool_r.is_err());
    auto bool_err = bool_r.unwrap_err();
    const auto *bool_info = std::get_if<TypeConversionError>(&bool_err.info());
    REQUIRE(bool_info != nullptr);
    CHECK(bool_info->expected == ExpectedType::Bool);
    CHECK(bool_info->expected_type == "bool (true/false/yes/no/1/0)");
    CHECK(
        std::string_view(bool_err.what()) ==
        "Parse Error: invalid value 'bad' for '--flag': expected "
        "bool (true/false/yes/no/1/0)");
}

TEST_CASE("Custom Converter specialization keeps free-form expected_type")
{
    using pjh::cli::Converter;
    using pjh::cli::ExpectedType;
    using pjh::cli::TypeConversionError;

    auto r = Converter<Widget>::from_string("bad", "--w");
    REQUIRE(r.is_err());
    auto err = r.unwrap_err();
    const auto *info = std::get_if<TypeConversionError>(&err.info());
    REQUIRE(info != nullptr);
    CHECK(info->expected == ExpectedType::Unknown);
    CHECK(info->expected_type == "widget");
    CHECK(
        std::string_view(err.what()) ==
        "Parse Error: invalid value 'bad' for '--w': expected widget");
}
