#include <doctest/doctest.h>

#include <iostream>
#include <filesystem>
#include <pjh_cli/app.hpp>
#include <pjh_cli/core/fixed_string.hpp>

#include "test_helpers.hpp"

namespace fs = std::filesystem;

TEST_CASE("Parser empty args")
{
    App app("test", "1.0", "Empty test");
    Argv argv{"test"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_ok());
}

TEST_CASE("Parser simple long option")
{
    App app("test", "1.0", "Long option test");
    app.option<fixed_string("port")>("--port", "Port number").integer();
    Argv argv{"test", "--port", "8080"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_ok());
    auto val = r.unwrap().get<int, fixed_string("port")>();
    CHECK(val == 8080);
}

TEST_CASE("Parser long option with equals value")
{
    App app("test", "1.0", "Equals test");
    app.option<fixed_string("port")>("--port", "Port number").integer();
    Argv argv{"test", "--port=8080"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_ok());
    auto val = r.unwrap().get<int, fixed_string("port")>();
    CHECK(val == 8080);
}

TEST_CASE("Parser long option with empty equals")
{
    App app("test", "1.0", "Empty equals test");
    app.option<fixed_string("port")>("--port", "Port number").integer();
    Argv argv{"test", "--port="};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_err());
}

TEST_CASE("Parser short option next token")
{
    App app("test", "1.0", "Short test");
    app.option<fixed_string("port")>("--port", 'p', "Port").integer();
    Argv argv{"test", "-p", "8080"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_ok());
    auto val = r.unwrap().get<int, fixed_string("port")>();
    CHECK(val == 8080);
}

TEST_CASE("Parser multiple options")
{
    App app("test", "1.0", "Multiple test");
    app.option<fixed_string("port")>("--port", "Port").integer();
    app.option<fixed_string("verbose")>("--verbose", "Verbose").boolean();
    Argv argv{"test", "--port", "8080", "--verbose"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_ok());
    auto &ctx = r.unwrap();
    auto port = ctx.get<int, fixed_string("port")>();
    CHECK(port == 8080);
    auto verb = ctx.get<bool, fixed_string("verbose")>();
    CHECK(verb == true);
}

TEST_CASE("Parser default applied when option not provided")
{
    App app("test", "1.0", "Default test");
    app.option<fixed_string("port")>("--port", "Port", 3000);
    Argv argv{"test"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_ok());
    auto val = r.unwrap().get<int, fixed_string("port")>();
    CHECK(val == 3000);
}

TEST_CASE("Parser option value overrides default")
{
    App app("test", "1.0", "Override test");
    app.option<fixed_string("port")>("--port", "Port", 3000);
    Argv argv{"test", "--port", "8080"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_ok());
    auto val = r.unwrap().get<int, fixed_string("port")>();
    CHECK(val == 8080);
}

TEST_CASE("Parser equals form type conversion failure")
{
    App app("test", "1.0", "Equals conversion");
    app.option<fixed_string("port")>("--port", "Port").integer();
    Argv argv{"test", "--port=notanumber"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_err());
}

TEST_CASE("Parser short mixed bool flag then valued option")
{
    App app("test", "1.0", "Mixed short");
    app.option<fixed_string("verbose")>("--verbose", 'v', "Verbose").boolean();
    app.option<fixed_string("port")>("--port", 'p', "Port").integer();
    Argv argv{"test", "-vp", "8080"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_ok());
    auto &ctx = r.unwrap();
    CHECK(ctx.get<bool, fixed_string("verbose")>() == true);
    CHECK(ctx.get<int, fixed_string("port")>() == 8080);
}

TEST_CASE("Parser short mixed valued option then bool flag errors")
{
    App app("test", "1.0", "Mixed short");
    app.option<fixed_string("port")>("--port", 'p', "Port").integer();
    app.option<fixed_string("verbose")>("--verbose", 'v', "Verbose").boolean();
    Argv argv{"test", "-pv", "8080"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_err());
}

TEST_CASE("Parser compact short option -p8080")
{
    App app("test", "1.0", "Compact test");
    app.option<fixed_string("port")>("--port", 'p', "Port").integer();
    Argv argv{"test", "-p8080"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_ok());
    auto val = r.unwrap().get<int, fixed_string("port")>();
    CHECK(val == 8080);
}

TEST_CASE("Parser compact short with flag group -vp consumes next token")
{
    App app("test", "1.0", "Compact group");
    app.option<fixed_string("verbose")>("--verbose", 'v', "Verbose").boolean();
    app.option<fixed_string("port")>("--port", 'p', "Port").integer();
    Argv argv{"test", "-vp", "9090"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_ok());
    auto &ctx = r.unwrap();
    CHECK(ctx.get<bool, fixed_string("verbose")>() == true);
    CHECK(ctx.get<int, fixed_string("port")>() == 9090);
}

TEST_CASE("Parser compact short -p8080 works alongside another option")
{
    App app("test", "1.0", "Compact alongside");
    app.option<fixed_string("port")>("--port", 'p', "Port").integer();
    app.option<fixed_string("verbose")>("--verbose", 'v', "Verbose").boolean();
    Argv argv{"test", "-p8080", "-v"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_ok());
    auto &ctx = r.unwrap();
    CHECK(ctx.get<int, fixed_string("port")>() == 8080);
    CHECK(ctx.get<bool, fixed_string("verbose")>() == true);
}

TEST_CASE("Parser compact short errors when value is non-numeric for int")
{
    App app("test", "1.0", "Compact bad");
    app.option<fixed_string("port")>("--port", 'p', "Port").integer();
    Argv argv{"test", "-pabc"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_err());
}

TEST_CASE("Parser compact short with repeatable greedy")
{
    App app("test", "1.0", "Compact greedy");
    app.option<fixed_string("files")>("--files", 'f', "Files").path().repeatable();
    Argv argv{"test", "-fa.txt", "b.txt", "c.txt"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_ok());
    auto all = r.unwrap().get_all<fs::path, fixed_string("files")>();
    REQUIRE(all.size() == 3);
    CHECK(all[0] == "a.txt");
    CHECK(all[1] == "b.txt");
    CHECK(all[2] == "c.txt");
}

TEST_CASE("Parser short valued option alone works")
{
    App app("test", "1.0", "Separate test");
    app.option<fixed_string("port")>("--port", 'p', "Port").integer();
    Argv argv{"test", "-p", "8080"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_ok());
    auto val = r.unwrap().get<int, fixed_string("port")>();
    CHECK(val == 8080);
}

TEST_CASE("Parser short flags group works")
{
    App app("test", "1.0", "Flag group");
    app.option<fixed_string("verbose")>("--verbose", 'v', "Verbose").boolean();
    app.option<fixed_string("force")>("--force", 'f', "Force").boolean();
    Argv argv{"test", "-vf"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_ok());
    auto &ctx = r.unwrap();
    CHECK(ctx.get<bool, fixed_string("verbose")>() == true);
    CHECK(ctx.get<bool, fixed_string("force")>() == true);
}

TEST_CASE("Parser valued option in group middle errors")
{
    App app("test", "1.0", "Group middle");
    app.option<fixed_string("verbose")>("--verbose", 'v', "Verbose").boolean();
    app.option<fixed_string("port")>("--port", 'p', "Port").integer();
    app.option<fixed_string("force")>("--force", 'f', "Force").boolean();
    Argv argv{"test", "-vpf", "8080"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_err());
}

// ── repeatable greedy multi-value consumption ──

TEST_CASE("Repeatable greedy consumes multiple values after --opt")
{
    App app("test", "1.0", "Greedy long");
    app.option<fixed_string("files")>("--files", 'f', "Files").path().repeatable();
    Argv argv{"test", "--files", "a.txt", "b.txt", "c.txt"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_ok());
    auto all = r.unwrap().get_all<fs::path, fixed_string("files")>();
    REQUIRE(all.size() == 3);
    CHECK(all[0] == "a.txt");
    CHECK(all[1] == "b.txt");
    CHECK(all[2] == "c.txt");
}

TEST_CASE("Repeatable greedy stops at next option flag")
{
    App app("test", "1.0", "Greedy stop");
    app.option<fixed_string("files")>("--files", 'f', "Files").path().repeatable();
    app.option<fixed_string("verbose")>("--verbose", 'v', "Verbose").boolean();
    Argv argv{"test", "--files", "a.txt", "--verbose"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_ok());
    auto &ctx = r.unwrap();
    auto all = ctx.get_all<fs::path, fixed_string("files")>();
    REQUIRE(all.size() == 1);
    CHECK(all[0] == "a.txt");
    CHECK(ctx.get<bool, fixed_string("verbose")>() == true);
}

TEST_CASE("Repeatable greedy works with short options")
{
    App app("test", "1.0", "Greedy short");
    app.option<fixed_string("files")>("--files", 'f', "Files").path().repeatable();
    Argv argv{"test", "-f", "x.txt", "y.txt", "z.txt"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_ok());
    auto all = r.unwrap().get_all<fs::path, fixed_string("files")>();
    REQUIRE(all.size() == 3);
    CHECK(all[0] == "x.txt");
    CHECK(all[1] == "y.txt");
    CHECK(all[2] == "z.txt");
}

TEST_CASE("Repeatable greedy stops at short option flag")
{
    App app("test", "1.0", "Greedy stop short");
    app.option<fixed_string("files")>("--files", 'f', "Files").path().repeatable();
    app.option<fixed_string("verbose")>("--verbose", 'v', "Verbose").boolean();
    Argv argv{"test", "-f", "a.txt", "-v"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_ok());
    auto &ctx = r.unwrap();
    auto all = ctx.get_all<fs::path, fixed_string("files")>();
    REQUIRE(all.size() == 1);
    CHECK(all[0] == "a.txt");
    CHECK(ctx.get<bool, fixed_string("verbose")>() == true);
}

TEST_CASE("Repeatable greedy stops at = form")
{
    App app("test", "1.0", "Greedy equals");
    app.option<fixed_string("files")>("--files", "Files").path().repeatable();
    Argv argv{"test", "--files=a.txt", "b.txt"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_ok());
    auto &ctx = r.unwrap();
    auto all = ctx.get_all<fs::path, fixed_string("files")>();
    REQUIRE(all.size() == 1);
    CHECK(all[0] == "a.txt");
}

TEST_CASE("Repeatable greedy stops at -- terminator")
{
    App app("test", "1.0", "Greedy dash");
    app.set_extra_args(ExtraArgsPolicy::Store);
    app.option<fixed_string("files")>("--files", "Files").path().repeatable();
    Argv argv{"test", "--files", "a.txt", "--", "b.txt"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_ok());
    auto &ctx = r.unwrap();
    auto all = ctx.get_all<fs::path, fixed_string("files")>();
    REQUIRE(all.size() == 1);
    CHECK(all[0] == "a.txt");
    REQUIRE(ctx.extra_args().size() == 1);
    CHECK(ctx.extra_args()[0] == "b.txt");
}

TEST_CASE("Repeatable greedy with mixed forms consolidates all values")
{
    App app("test", "1.0", "Mixed greedy");
    app.option<fixed_string("files")>("--files", 'f', "Files").path().repeatable();
    Argv argv{"test", "--files", "a.txt", "b.txt", "-f", "c.txt", "d.txt"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_ok());
    auto all = r.unwrap().get_all<fs::path, fixed_string("files")>();
    REQUIRE(all.size() == 4);
    CHECK(all[0] == "a.txt");
    CHECK(all[1] == "b.txt");
    CHECK(all[2] == "c.txt");
    CHECK(all[3] == "d.txt");
}

TEST_CASE("Non-repeatable option still requires exactly one value")
{
    App app("test", "1.0", "Single strict");
    app.option<fixed_string("port")>("--port", "Port").integer();
    Argv argv{"test", "--port", "8080", "9090"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_ok());
    auto &ctx = r.unwrap();
    CHECK(ctx.get<int, fixed_string("port")>() == 8080);
}

TEST_CASE("Non-repeatable option missing value errors")
{
    App app("test", "1.0", "Missing val");
    app.option<fixed_string("port")>("--port", "Port").integer();
    Argv argv{"test", "--port"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_err());
}

TEST_CASE("Non-repeatable next token is option flag errors")
{
    App app("test", "1.0", "Dash val");
    app.option<fixed_string("port")>("--port", "Port").integer();
    Argv argv{"test", "--port", "--verbose"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_err());
}

// ── negative number values ──

TEST_CASE("Negative integer --offset -5")
{
    App app("test", "1.0", "Neg int");
    app.option<fixed_string("offset")>("--offset", 'o', "Offset").integer();
    Argv argv{"test", "--offset", "-5"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_ok());
    CHECK(r.unwrap().get<int, fixed_string("offset")>() == -5);
}

TEST_CASE("Negative integer short -o -5")
{
    App app("test", "1.0", "Neg short");
    app.option<fixed_string("offset")>("--offset", 'o', "Offset").integer();
    Argv argv{"test", "-o", "-5"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_ok());
    CHECK(r.unwrap().get<int, fixed_string("offset")>() == -5);
}

TEST_CASE("Negative integer compact -o-5")
{
    App app("test", "1.0", "Neg compact");
    app.option<fixed_string("offset")>("--offset", 'o', "Offset").integer();
    Argv argv{"test", "-o-5"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_ok());
    CHECK(r.unwrap().get<int, fixed_string("offset")>() == -5);
}

TEST_CASE("Negative float --ratio -3.14")
{
    App app("test", "1.0", "Neg float");
    app.option<fixed_string("ratio")>("--ratio", 'r', "Ratio").floating();
    Argv argv{"test", "--ratio", "-3.14"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_ok());
    CHECK(r.unwrap().get<double, fixed_string("ratio")>() == doctest::Approx(-3.14));
}

TEST_CASE("Option-flag next token still errors for valued option")
{
    App app("test", "1.0", "Flag next");
    app.option<fixed_string("offset")>("--offset", 'o', "Offset").integer();
    app.option<fixed_string("verbose")>("--verbose", 'v', "Verbose").boolean();
    Argv argv{"test", "--offset", "--verbose"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_err());
}

TEST_CASE("Repeatable greedily consumes negative numbers")
{
    App app("test", "1.0", "Neg repeat");
    app.option<fixed_string("nums")>("--nums", 'n', "Nums").integer().repeatable();
    Argv argv{"test", "--nums", "-5", "-3", "0", "7"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_ok());
    auto all = r.unwrap().get_all<int, fixed_string("nums")>();
    REQUIRE(all.size() == 4);
    CHECK(all[0] == -5);
    CHECK(all[1] == -3);
    CHECK(all[2] == 0);
    CHECK(all[3] == 7);
}

// ── enum option ──

enum class Color
{
    red,
    green,
    blue
};

TEST_CASE("EnumOption basic mapping")
{
    App app("test", "1.0", "Enum basic");
    app.option<fixed_string("color")>("--color", 'c', "Color")
        .enum_type<Color>()
        .mapping({{"red", Color::red}, {"green", Color::green}});
    Argv argv{"test", "--color", "red"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_ok());
    auto v = r.unwrap().get_enum<Color, fixed_string("color")>();
    CHECK(v == Color::red);
}

TEST_CASE("EnumOption invalid value errors")
{
    App app("test", "1.0", "Enum err");
    app.option<fixed_string("color")>("--color", 'c', "Color")
        .enum_type<Color>()
        .mapping({{"red", Color::red}, {"green", Color::green}});
    Argv argv{"test", "--color", "blue"};
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
    auto v = r.unwrap().get_enum<Color, fixed_string("color")>();
    CHECK(v == Color::green);
}

TEST_CASE("EnumOption repeatable collects multiple values")
{
    App app("test", "1.0", "Enum repeat");
    app.option<fixed_string("colors")>("--colors", 'c', "Colors")
        .enum_type<Color>()
        .mapping({{"red", Color::red}, {"green", Color::green}, {"blue", Color::blue}})
        .repeatable();
    Argv argv{"test", "--colors", "red", "green", "blue"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_ok());
    auto all = r.unwrap().get_all_enum<Color, fixed_string("colors")>();
    REQUIRE(all.size() == 3);
    CHECK(all[0] == Color::red);
    CHECK(all[1] == Color::green);
    CHECK(all[2] == Color::blue);
}
