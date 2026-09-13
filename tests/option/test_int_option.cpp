#include <doctest/doctest.h>

#include <initializer_list>
#include <iostream>
#include <pjh_cli/app.hpp>
#include <pjh_cli/core/fixed_string.hpp>
#include <string>
#include <string_view>
#include <vector>

#include "../argv.hpp"

using namespace pjh::cli;

// ── Basic IntOption behavior ──

TEST_CASE("IntOption parse_value stores int")
{
    App app("test", "1.0", "Int");
    app.option<fixed_string("port")>("--port", "Port").integer();
    Argv argv{"test", "--port", "8080"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_ok());
    CHECK(r.unwrap().get<int, fixed_string("port")>() == 8080);
}

TEST_CASE("IntOption default_value applied when absent")
{
    App app("test", "1.0", "Int default");
    app.option<fixed_string("port")>("--port", "Port").integer().default_value(3000);
    Argv argv{"test"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_ok());
    CHECK(r.unwrap().get<int, fixed_string("port")>() == 3000);
}

TEST_CASE("IntOption default_value overridden by CLI")
{
    App app("test", "1.0", "Int override");
    app.option<fixed_string("port")>("--port", "Port").integer().default_value(3000);
    Argv argv{"test", "--port", "8080"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_ok());
    CHECK(r.unwrap().get<int, fixed_string("port")>() == 8080);
}

TEST_CASE("IntOption default_value_str renders via BuiltinTraits")
{
    App app("test", "1.0", "Int default str");
    app.option<fixed_string("port")>("--port", "Port").integer().default_value(3000);
    auto *def = app.find_option_by_long("port");
    REQUIRE(def != nullptr);
    CHECK(def->default_value_str() == "3000");
}

TEST_CASE("IntOption has_default reflects default_value")
{
    App app("test", "1.0", "Int has_default");
    app.option<fixed_string("port")>("--port", "Port").integer().default_value(80);
    auto *def = app.find_option_by_long("port");
    CHECK(def != nullptr);
    CHECK(def->has_default());
}

TEST_CASE("IntOption no default means has_default false")
{
    App app("test", "1.0", "Int no default");
    app.option<fixed_string("port")>("--port", "Port").integer();
    auto *def = app.find_option_by_long("port");
    CHECK(def != nullptr);
    CHECK_FALSE(def->has_default());
}

// ── min / max ──

TEST_CASE("IntOption min-max within range ok")
{
    App app("test", "1.0", "MinMax");
    app.option<fixed_string("port")>("--port", "Port").integer().min(1).max(65535);
    Argv argv{"test", "--port", "8080"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_ok());
    CHECK(r.unwrap().get<int, fixed_string("port")>() == 8080);
}

TEST_CASE("IntOption min-max below min fails")
{
    App app("test", "1.0", "MinMax");
    app.option<fixed_string("port")>("--port", "Port").integer().min(1).max(65535);
    Argv argv{"test", "--port", "0"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_err());
}

TEST_CASE("IntOption min-max above max fails")
{
    App app("test", "1.0", "MinMax");
    app.option<fixed_string("port")>("--port", "Port").integer().min(1).max(65535);
    Argv argv{"test", "--port", "70000"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_err());
}

TEST_CASE("IntOption min only enforces lower bound")
{
    App app("test", "1.0", "Min only");
    app.option<fixed_string("level")>("--level", "Level").integer().min(0);
    Argv argv{"test", "--level", "-1"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_err());
    Argv argv2{"test", "--level", "0"};
    CHECK(app.parse(argv2.argc(), argv2.argv()).is_ok());
}

TEST_CASE("IntOption max only enforces upper bound")
{
    App app("test", "1.0", "Max only");
    app.option<fixed_string("level")>("--level", "Level").integer().max(10);
    Argv argv{"test", "--level", "11"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_err());
    Argv argv2{"test", "--level", "10"};
    CHECK(app.parse(argv2.argc(), argv2.argv()).is_ok());
}

TEST_CASE("IntOption min-max inclusive boundaries")
{
    App app("test", "1.0", "Boundaries");
    app.option<fixed_string("x")>("--x", "X").integer().min(1).max(3);
    Argv a1{"test", "--x", "1"};
    CHECK(app.parse(a1.argc(), a1.argv()).is_ok());
    Argv a2{"test", "--x", "2"};
    CHECK(app.parse(a2.argc(), a2.argv()).is_ok());
    Argv a3{"test", "--x", "3"};
    CHECK(app.parse(a3.argc(), a3.argv()).is_ok());
}

TEST_CASE("IntOption min-max error message format")
{
    App app("test", "1.0", "Err msg");
    app.option<fixed_string("port")>("--port", "Port").integer().min(1).max(65535);
    Argv argv{"test", "--port", "0"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_err());
    CHECK(
        r.unwrap_err().what() ==
        std::string_view(
            "Parse Error: value '0' for '--port' is out of range [1, 65535]"));
}

// ── repeatable ──

TEST_CASE("IntOption repeatable appends multiple values")
{
    App app("test", "1.0", "Repeat int");
    app.option<fixed_string("port")>("--port", 'p', "Port")
        .integer()
        .min(1)
        .max(65535)
        .repeatable();
    Argv argv{"test", "-p", "8080", "-p", "9090"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_ok());
    auto all = r.unwrap().get_all<int, fixed_string("port")>();
    REQUIRE(all.size() == 2);
    CHECK(all[0] == 8080);
    CHECK(all[1] == 9090);
}

TEST_CASE("IntOption repeatable validates each value with min")
{
    App app("test", "1.0", "Repeat min");
    app.option<fixed_string("port")>("--port", 'p', "Port")
        .integer()
        .min(100)
        .repeatable();
    Argv argv{"test", "-p", "200", "-p", "50"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_err());
    Argv argv2{"test", "-p", "200", "-p", "300"};
    CHECK(app.parse(argv2.argc(), argv2.argv()).is_ok());
}

// ── default validation through the yield/apply seam ──

TEST_CASE("IntOption default outside range is rejected")
{
    App app("test", "1.0", "Int default range");
    app.option<fixed_string("x")>("--x", "X").integer().min(10).default_value(5);
    Argv argv{"test"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_err());
    CHECK(
        r.unwrap_err().what() ==
        std::string_view(
            "Parse Error: value '' for '--x' is out of range [10, 2147483647]"));
}

TEST_CASE("IntOption repeatable default appends one value")
{
    App app("test", "1.0", "Repeat default");
    app.option<fixed_string("x")>("--x", "X").integer().repeatable().default_value(7);
    Argv argv{"test"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_ok());
    auto all = r.unwrap().get_all<int, fixed_string("x")>();
    REQUIRE(all.size() == 1);
    CHECK(all[0] == 7);
}

TEST_CASE("IntOption accepts leading plus value")
{
    App app("test", "1.0", "Int plus");
    app.option<fixed_string("port")>("--port", "Port").integer();
    Argv argv{"test", "--port", "+8080"};
    auto r = app.parse(argv.argc(), argv.argv());
    REQUIRE(r.is_ok());
    CHECK(r.unwrap().get<int, fixed_string("port")>() == 8080);
}

TEST_CASE("IntOption leading plus with range")
{
    App app("test", "1.0", "Int plus range");
    app.option<fixed_string("port")>("--port", "Port").integer().min(1).max(65535);
    Argv argv{"test", "--port", "+80"};
    auto r = app.parse(argv.argc(), argv.argv());
    REQUIRE(r.is_ok());
    CHECK(r.unwrap().get<int, fixed_string("port")>() == 80);
    Argv argv2{"test", "--port", "+0"};
    CHECK(app.parse(argv2.argc(), argv2.argv()).is_err());
}

TEST_CASE("IntOption lone plus is rejected")
{
    App app("test", "1.0", "Int lone plus");
    app.option<fixed_string("port")>("--port", "Port").integer();
    Argv argv{"test", "--port", "+"};
    auto r = app.parse(argv.argc(), argv.argv());
    REQUIRE(r.is_err());
    CHECK(
        r.unwrap_err().what() ==
        std::string_view(
            "Parse Error: invalid value '+' for '--port': expected integer"));
}
