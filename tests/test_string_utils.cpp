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

TEST_CASE("is_option_flag distinguishes option tokens from values")
{
    CHECK(is_option_flag("-x"));
    CHECK(is_option_flag("--verbose"));
    CHECK(is_option_flag("--port=8080"));
    CHECK(is_option_flag("--"));
    CHECK_FALSE(is_option_flag("-5"));
    CHECK_FALSE(is_option_flag("-3.14"));
    CHECK_FALSE(is_option_flag("-.5"));
    CHECK_FALSE(is_option_flag("-"));
    CHECK_FALSE(is_option_flag("word"));
    CHECK_FALSE(is_option_flag(""));
}

// 中 = E4 B8 AD, 文 = E6 96 87, emoji 😀 = F0 9F 98 80.

TEST_CASE("utf8_prev_code_point walks back over continuation bytes")
{
    const std::string s = "\xE4\xB8\xAD";  // 中
    CHECK(utf8_prev_code_point(s, s.size()) == 0);
    CHECK(utf8_prev_code_point(s, 1) == 0);
    CHECK(utf8_prev_code_point("abc", 3) == 2);
    CHECK(utf8_prev_code_point("abc", 1) == 0);
    CHECK(utf8_prev_code_point("", 0) == 0);
}

TEST_CASE("utf8_prev_code_point handles mixed and 4-byte sequences")
{
    const std::string mixed = std::string("a") + "\xE4\xB8\xAD";  // a中
    CHECK(utf8_prev_code_point(mixed, mixed.size()) == 1);
    CHECK(utf8_prev_code_point(mixed, 1) == 0);
    const std::string emoji = "\xF0\x9F\x98\x80";  // 😀
    CHECK(utf8_prev_code_point(emoji, emoji.size()) == 0);
}

TEST_CASE("utf8_prev_code_point is safe on malformed input")
{
    const std::string lone = "\x80";     // stray continuation byte
    const std::string run = "\x80\x80";  // two stray bytes
    CHECK(utf8_prev_code_point(lone, lone.size()) == 0);
    CHECK(utf8_prev_code_point(run, run.size()) == 0);
    CHECK(utf8_prev_code_point("a", 99) == 0);  // end clamps to size
}

TEST_CASE("utf8_code_point_count counts code points not bytes")
{
    CHECK(utf8_code_point_count("") == 0);
    CHECK(utf8_code_point_count("abc") == 3);
    CHECK(utf8_code_point_count("\xE4\xB8\xAD") == 1);              // 中
    CHECK(utf8_code_point_count("\xE4\xB8\xAD\xE6\x96\x87") == 2);  // 中文
    CHECK(utf8_code_point_count(std::string("a") + "\xE4\xB8\xAD") == 2);
    CHECK(utf8_code_point_count("\x80\x80") == 0);  // stray continuations
}
