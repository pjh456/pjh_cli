#include <pjh_cli.hpp>
#include <cassert>
#include <string_view>

int main()
{
    using pjh::cli::fixed_string;

    constexpr fixed_string fs("hello");
    static_assert(fs.size() == 5);
    static_assert(fs.view() == std::string_view("hello"));
    static_assert(fs.value[0] == 'h');
    static_assert(fs.value[5] == '\0');

    constexpr auto eq = fixed_string("abc") == fixed_string("abc");
    static_assert(eq);

    constexpr auto neq = fixed_string("abc") == fixed_string("xyz");
    static_assert(!neq);

    constexpr fixed_string empty("");
    static_assert(empty.size() == 0);
    static_assert(empty.view() == std::string_view(""));

    constexpr fixed_string num("12345");
    static_assert(num.view() == "12345");
    static_assert(num.value[4] == '5');

    // ── Cross-size comparison (N != N2) ──
    constexpr fixed_string hello("hello");
    constexpr fixed_string hello_big("hello!");

    // Different lengths → not equal
    constexpr auto cross1 = hello == hello_big;
    static_assert(!cross1);

    // Same content vs different content, cross-size
    constexpr auto cross2 = hello == fixed_string("world");
    static_assert(!cross2);

    // Short vs long, same prefix
    constexpr fixed_string ab("ab");
    constexpr fixed_string abc("abc");
    constexpr auto cross3 = ab == abc;
    static_assert(!cross3);

    // Cross-size same string viewed from different-length wrappers
    // (structurally impossible: different N always means different content)
    constexpr auto cross4 = fixed_string("xy") == fixed_string("xyz");
    static_assert(!cross4);

    // Ensure same-size still works alongside cross-size
    constexpr auto same = fixed_string("abc") == fixed_string("abc");
    static_assert(same);

    constexpr auto same_not = fixed_string("abc") == fixed_string("abx");
    static_assert(!same_not);

    // Empty vs non-empty cross-size
    constexpr auto cross_empty = empty == fixed_string("x");
    static_assert(!cross_empty);

    return 0;
}
