#include <doctest/doctest.h>

#include <iostream>
#include <pjh_cli/detail/string_utils.hpp>
#include <string>
#include <string_view>

using namespace pjh::cli::detail;

TEST_CASE("to_upper_copy all lowercase")
{
    CHECK(StringUtils::to_upper_copy("hello") == "HELLO");
}

TEST_CASE("to_upper_copy already upper")
{
    CHECK(StringUtils::to_upper_copy("HELLO") == "HELLO");
}

TEST_CASE("to_upper_copy mixed")
{
    CHECK(StringUtils::to_upper_copy("HeLLo WoRLd") == "HELLO WORLD");
}

TEST_CASE("to_upper_copy digits and symbols")
{
    CHECK(StringUtils::to_upper_copy("port123-xyz") == "PORT123-XYZ");
}

TEST_CASE("to_upper_copy empty") { CHECK(StringUtils::to_upper_copy("").empty()); }

TEST_CASE("to_upper_copy single char")
{
    CHECK(StringUtils::to_upper_copy("a") == "A");
    CHECK(StringUtils::to_upper_copy("Z") == "Z");
}

TEST_CASE("case_insensitive_equal same case")
{
    CHECK(StringUtils::case_insensitive_equal("abc", "abc") == true);
}

TEST_CASE("case_insensitive_equal different case")
{
    CHECK(StringUtils::case_insensitive_equal("abc", "ABC") == true);
    CHECK(StringUtils::case_insensitive_equal("AbC", "aBc") == true);
}

TEST_CASE("case_insensitive_equal different strings")
{
    CHECK(StringUtils::case_insensitive_equal("abc", "xyz") == false);
}

TEST_CASE("case_insensitive_equal different lengths")
{
    CHECK(StringUtils::case_insensitive_equal("abc", "abcd") == false);
}

TEST_CASE("case_insensitive_equal empty")
{
    CHECK(StringUtils::case_insensitive_equal("", "") == true);
    CHECK(StringUtils::case_insensitive_equal("", "a") == false);
}

TEST_CASE("case_insensitive_equal bool keywords")
{
    CHECK(StringUtils::case_insensitive_equal("true", "TRUE") == true);
    CHECK(StringUtils::case_insensitive_equal("yes", "YES") == true);
    CHECK(StringUtils::case_insensitive_equal("false", "FALSE") == true);
    CHECK(StringUtils::case_insensitive_equal("no", "NO") == true);
}

TEST_CASE("split_name_value simple")
{
    auto r = StringUtils::split_name_value("port=8080");
    CHECK(r.name == "port");
    CHECK(r.value == "8080");
    CHECK(r.has_eq == true);
}

TEST_CASE("split_name_value empty value")
{
    auto r = StringUtils::split_name_value("port=");
    CHECK(r.name == "port");
    CHECK(r.value == "");
    CHECK(r.has_eq == true);
}

TEST_CASE("split_name_value empty name")
{
    auto r = StringUtils::split_name_value("=8080");
    CHECK(r.name == "");
    CHECK(r.value == "8080");
    CHECK(r.has_eq == true);
}

TEST_CASE("split_name_value no equals")
{
    auto r = StringUtils::split_name_value("port");
    CHECK(r.name == "port");
    CHECK(r.value == "");
    CHECK(r.has_eq == false);
}

TEST_CASE("split_name_value empty string")
{
    auto r = StringUtils::split_name_value("");
    CHECK(r.name == "");
    CHECK(r.value == "");
    CHECK(r.has_eq == false);
}

TEST_CASE("split_name_value with dashes")
{
    auto r = StringUtils::split_name_value("--port=8080");
    CHECK(r.name == "--port");
    CHECK(r.value == "8080");
    CHECK(r.has_eq == true);
}

TEST_CASE("split_name_value multiple equals")
{
    auto r = StringUtils::split_name_value("a=b=c");
    CHECK(r.name == "a");
    CHECK(r.value == "b=c");
    CHECK(r.has_eq == true);
}

TEST_CASE("split_name_value just equals")
{
    auto r = StringUtils::split_name_value("=");
    CHECK(r.name == "");
    CHECK(r.value == "");
    CHECK(r.has_eq == true);
}
