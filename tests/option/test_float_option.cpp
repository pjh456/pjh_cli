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

TEST_CASE("FloatOption parse_value stores double")
{
    App app("test", "1.0", "Float");
    app.option<fixed_string("rate")>("--rate", "Rate").floating();
    Argv argv{"test", "--rate", "3.14"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_ok());
    CHECK(r.unwrap().get<double, fixed_string("rate")>() == doctest::Approx(3.14));
}

TEST_CASE("FloatOption default_value applied when absent")
{
    App app("test", "1.0", "Float default");
    app.option<fixed_string("rate")>("--rate", "Rate").floating().default_value(2.5);
    Argv argv{"test"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_ok());
    CHECK(r.unwrap().get<double, fixed_string("rate")>() == doctest::Approx(2.5));
}

TEST_CASE("FloatOption has_default reflects default_value")
{
    App app("test", "1.0", "Float has_default");
    app.option<fixed_string("rate")>("--rate", "Rate").floating().default_value(1.0);
    auto *def = app.find_option_by_long("rate");
    CHECK(def != nullptr);
    CHECK(def->has_default());
}

TEST_CASE("FloatOption min-max within range ok")
{
    App app("test", "1.0", "Float range");
    app.option<fixed_string("rate")>("--rate", "Rate").floating().min(0.0).max(1.0);
    Argv argv{"test", "--rate", "0.5"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_ok());
    CHECK(r.unwrap().get<double, fixed_string("rate")>() == doctest::Approx(0.5));
}

TEST_CASE("FloatOption below min fails")
{
    App app("test", "1.0", "Float below");
    app.option<fixed_string("rate")>("--rate", "Rate").floating().min(0.0);
    Argv argv{"test", "--rate", "-0.1"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_err());
}

TEST_CASE("FloatOption above max fails")
{
    App app("test", "1.0", "Float above");
    app.option<fixed_string("rate")>("--rate", "Rate").floating().max(1.0);
    Argv argv{"test", "--rate", "1.5"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_err());
}

TEST_CASE("FloatOption min inclusive at boundary")
{
    App app("test", "1.0", "Float boundary");
    app.option<fixed_string("rate")>("--rate", "Rate").floating().min(0.0).max(1.0);
    Argv a0{"test", "--rate", "0.0"};
    CHECK(app.parse(a0.argc(), a0.argv()).is_ok());
    Argv a1{"test", "--rate", "1.0"};
    CHECK(app.parse(a1.argc(), a1.argv()).is_ok());
}

TEST_CASE("FloatOption default_value_str renders via BuiltinTraits")
{
    App app("test", "1.0", "Float default str");
    app.option<fixed_string("rate")>("--rate", "Rate").floating().default_value(2.5);
    auto *def = app.find_option_by_long("rate");
    REQUIRE(def != nullptr);
    CHECK(def->default_value_str() == "2.500000");
}

TEST_CASE("FloatOption conversion error message names option and type")
{
    App app("test", "1.0", "Float err");
    app.option<fixed_string("rate")>("--rate", "Rate").floating();
    Argv argv{"test", "--rate", "x"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_err());
    CHECK(
        r.unwrap_err().what() ==
        std::string_view("Parse Error: invalid value 'x' for '--rate': expected float"));
}

TEST_CASE("FloatOption accepts leading plus value")
{
    App app("test", "1.0", "Float plus");
    app.option<fixed_string("rate")>("--rate", "Rate").floating();
    Argv argv{"test", "--rate", "+3.5"};
    auto r = app.parse(argv.argc(), argv.argv());
    REQUIRE(r.is_ok());
    CHECK(r.unwrap().get<double, fixed_string("rate")>() == doctest::Approx(3.5));
}

TEST_CASE("FloatOption rejects nan")
{
    App app("test", "1.0", "Float nan");
    app.option<fixed_string("rate")>("--rate", "Rate").floating();
    Argv argv{"test", "--rate", "nan"};
    auto r = app.parse(argv.argc(), argv.argv());
    REQUIRE(r.is_err());
    CHECK(
        r.unwrap_err().what() ==
        std::string_view(
            "Parse Error: invalid value 'nan' for '--rate': expected float"));
}

TEST_CASE("FloatOption rejects inf and negative inf")
{
    App app("test", "1.0", "Float inf");
    app.option<fixed_string("rate")>("--rate", "Rate").floating();
    Argv a1{"test", "--rate", "inf"};
    CHECK(app.parse(a1.argc(), a1.argv()).is_err());
    Argv a2{"test", "--rate", "-inf"};
    CHECK(app.parse(a2.argc(), a2.argv()).is_err());
    Argv a3{"test", "--rate", "infinity"};
    CHECK(app.parse(a3.argc(), a3.argv()).is_err());
}

TEST_CASE("FloatOption nan does not bypass range")
{
    App app("test", "1.0", "Float nan range");
    app.option<fixed_string("rate")>("--rate", "Rate").floating().min(0.0).max(1.0);
    Argv argv{"test", "--rate", "nan"};
    CHECK(app.parse(argv.argc(), argv.argv()).is_err());
}

TEST_CASE("FloatOption inf does not bypass min-only range")
{
    App app("test", "1.0", "Float inf min");
    app.option<fixed_string("rate")>("--rate", "Rate").floating().min(0.0);
    Argv argv{"test", "--rate", "inf"};
    CHECK(app.parse(argv.argc(), argv.argv()).is_err());
}
