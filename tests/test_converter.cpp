#include <doctest/doctest.h>

#include <iostream>
#include <pjh_cli/core/converter.hpp>
#include <string>
#include <string_view>

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
