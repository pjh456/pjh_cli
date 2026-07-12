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

    return 0;
}
