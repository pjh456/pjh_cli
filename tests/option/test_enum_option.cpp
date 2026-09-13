#include <doctest/doctest.h>

#include <initializer_list>
#include <pjh_cli/app.hpp>
#include <pjh_cli/core/fixed_string.hpp>
#include <string>
#include <vector>

#include "../argv.hpp"

using namespace pjh::cli;

namespace
{
    enum class Color
    {
        red,
        green,
        blue
    };

    enum class WideEnum : long long
    {
        x = 0x1'0000'0001LL
    };

    enum class Flags : unsigned
    {
        big = 0x80000000u
    };
}

static_assert(detail::IntStorableEnum<Color>);
static_assert(!detail::IntStorableEnum<WideEnum>);

TEST_CASE("EnumOption basic mapping")
{
    App app("test", "1.0", "Enum");
    app.option<fixed_string("color")>("--color", 'c', "Color")
        .enum_type<Color>()
        .mapping({{"red", Color::red}, {"green", Color::green}, {"blue", Color::blue}});
    Argv argv{"test", "--color", "red"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_ok());
    CHECK(r.unwrap().get_enum<Color, fixed_string("color")>() == Color::red);
}

TEST_CASE("EnumOption invalid value errors")
{
    App app("test", "1.0", "Enum err");
    app.option<fixed_string("color")>("--color", 'c', "Color")
        .enum_type<Color>()
        .mapping({{"red", Color::red}, {"green", Color::green}});
    Argv argv{"test", "--color", "yellow"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_err());
}

TEST_CASE("EnumOption default_value applied when absent")
{
    App app("test", "1.0", "Enum default");
    app.option<fixed_string("color")>("--color", 'c', "Color")
        .enum_type<Color>()
        .mapping({{"red", Color::red}, {"green", Color::green}})
        .default_value(Color::green);
    Argv argv{"test"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_ok());
    CHECK(r.unwrap().get_enum<Color, fixed_string("color")>() == Color::green);
}

TEST_CASE("EnumOption repeatable collects multiple values")
{
    App app("test", "1.0", "Enum repeat");
    app.option<fixed_string("color")>("--color", 'c', "Color")
        .enum_type<Color>()
        .mapping({{"red", Color::red}, {"green", Color::green}, {"blue", Color::blue}})
        .repeatable();
    Argv argv{"test", "--color", "red", "--color", "blue"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_ok());
    auto all = r.unwrap().get_all_enum<Color, fixed_string("color")>();
    REQUIRE(all.size() == 2);
    CHECK(all[0] == Color::red);
    CHECK(all[1] == Color::blue);
}

TEST_CASE("EnumOption repeatable get_all_enum and get_all<int> consistency")
{
    App app("test", "1.0", "Enum repeat int");
    app.option<fixed_string("color")>("--color", 'c', "Color")
        .enum_type<Color>()
        .mapping({{"red", Color::red}, {"green", Color::green}, {"blue", Color::blue}})
        .repeatable();
    Argv argv{"test", "--color", "red", "--color", "green"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_ok());
    auto &ctx = r.unwrap();
    auto enums = ctx.get_all_enum<Color, fixed_string("color")>();
    auto ints = ctx.get_all<int, fixed_string("color")>();
    REQUIRE(enums.size() == 2);
    REQUIRE(ints.size() == 2);
    CHECK(static_cast<int>(enums[0]) == ints[0]);
    CHECK(static_cast<int>(enums[1]) == ints[1]);
}

TEST_CASE("EnumOption preserves same-size unsigned enum values")
{
    App app("test", "1.0", "Enum wide");
    app.option<fixed_string("flags")>("--flags", "Flags")
        .enum_type<Flags>()
        .mapping({{"big", Flags::big}});
    Argv argv{"test", "--flags", "big"};
    auto r = app.parse(argv.argc(), argv.argv());
    REQUIRE(r.is_ok());
    CHECK(r.unwrap().get_enum<Flags, fixed_string("flags")>() == Flags::big);
}
