#include <doctest/doctest.h>

#include <pjh_cli/core/fixed_string.hpp>
#include <string_view>

TEST_CASE("fixed_string basic properties")
{
    using pjh::cli::fixed_string;

    constexpr fixed_string fs("hello");
    static_assert(fs.size() == 5);
    static_assert(fs.view() == std::string_view("hello"));
    static_assert(fs.value[0] == 'h');
    static_assert(fs.value[5] == '\0');
}

TEST_CASE("fixed_string empty")
{
    using pjh::cli::fixed_string;

    constexpr fixed_string empty("");
    static_assert(empty.size() == 0);
    static_assert(empty.view() == std::string_view(""));
}

TEST_CASE("fixed_string numeric")
{
    using pjh::cli::fixed_string;

    constexpr fixed_string num("12345");
    static_assert(num.view() == "12345");
    static_assert(num.value[4] == '5');
}

TEST_CASE("fixed_string equal same type")
{
    using pjh::cli::fixed_string;

    constexpr auto eq = fixed_string("abc") == fixed_string("abc");
    static_assert(eq);

    constexpr auto neq = fixed_string("abc") == fixed_string("xyz");
    static_assert(!neq);
}

TEST_CASE("fixed_string cross size compare")
{
    using pjh::cli::fixed_string;

    constexpr fixed_string hello("hello");
    constexpr fixed_string hello_big("hello!");
    constexpr auto cross1 = hello == hello_big;
    static_assert(!cross1);

    constexpr auto cross2 = hello == fixed_string("world");
    static_assert(!cross2);

    constexpr fixed_string ab("ab");
    constexpr fixed_string abc("abc");
    constexpr auto cross3 = ab == abc;
    static_assert(!cross3);

    constexpr auto cross4 = fixed_string("xy") == fixed_string("xyz");
    static_assert(!cross4);
}

TEST_CASE("fixed_string same size compare")
{
    using pjh::cli::fixed_string;

    constexpr auto same = fixed_string("abc") == fixed_string("abc");
    static_assert(same);

    constexpr auto same_not = fixed_string("abc") == fixed_string("abx");
    static_assert(!same_not);
}

TEST_CASE("fixed_string cross size with empty")
{
    using pjh::cli::fixed_string;

    constexpr fixed_string empty("");
    constexpr auto cross_empty = empty == fixed_string("x");
    static_assert(!cross_empty);
}
