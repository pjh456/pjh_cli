#include <doctest/doctest.h>

#include <initializer_list>
#include <iostream>
#include <pjh_cli/app.hpp>
#include <pjh_cli/core/fixed_string.hpp>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

#include "../argv.hpp"

using namespace pjh::cli;

static_assert(
    std::is_same_v<CountOption::ValueType, int>,
    "CountOption must derive its ValueType from WithDefault<int>");

TEST_CASE("CountOption short group increments")
{
    App app("test", "1.0", "Count");
    app.option<fixed_string("v")>("--verbose", 'v', "Verbosity").count();
    Argv argv{"test", "-vvv"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_ok());
    CHECK(r.unwrap().get<int, fixed_string("v")>() == 3);
}

TEST_CASE("CountOption separate shorts add up")
{
    App app("test", "1.0", "Count");
    app.option<fixed_string("v")>("--verbose", 'v', "Verbosity").count();
    Argv argv{"test", "-v", "-v"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_ok());
    CHECK(r.unwrap().get<int, fixed_string("v")>() == 2);
}

TEST_CASE("CountOption long form increments")
{
    App app("test", "1.0", "Count");
    app.option<fixed_string("v")>("--verbose", 'v', "Verbosity").count();
    Argv argv{"test", "--verbose", "--verbose", "--verbose"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_ok());
    CHECK(r.unwrap().get<int, fixed_string("v")>() == 3);
}

TEST_CASE("CountOption absent means no value")
{
    App app("test", "1.0", "Count");
    app.option<fixed_string("v")>("--verbose", 'v', "Verbosity").count();
    Argv argv{"test"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_ok());
    CHECK_FALSE(r.unwrap().has<fixed_string("v")>());
}

TEST_CASE("CountOption mixed with bool flags")
{
    App app("test", "1.0", "Count mixed");
    app.option<fixed_string("v")>("--verbose", 'v', "Verbosity").count();
    app.option<fixed_string("f")>("--flag", 'f', "Flag").boolean();
    Argv argv{"test", "-vff"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_ok());
    auto &ctx = r.unwrap();
    CHECK(ctx.get<int, fixed_string("v")>() == 1);
    CHECK(ctx.get<bool, fixed_string("f")>() == true);
}

TEST_CASE("CountOption rejects --opt=value")
{
    App app("test", "1.0", "Count eq");
    app.option<fixed_string("v")>("--verbose", 'v', "Verbosity").count();
    Argv argv{"test", "--verbose=x"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_err());
    CHECK(
        r.unwrap_err().what() ==
        std::string_view("Parse Error: option '--verbose' does not accept a value"));
}

TEST_CASE("CountOption default_value applied when absent")
{
    App app("test", "1.0", "Count default");
    app.option<fixed_string("v")>("--verbose", 'v', "Verbosity").count().default_value(2);
    Argv argv{"test"};
    auto r = app.parse(argv.argc(), argv.argv());
    REQUIRE(r.is_ok());
    CHECK(r.unwrap().get<int, fixed_string("v")>() == 2);
}

TEST_CASE("CountOption CLI occurrences override default")
{
    App app("test", "1.0", "Count default CLI");
    app.option<fixed_string("v")>("--verbose", 'v', "Verbosity").count().default_value(5);
    Argv argv{"test", "-vv"};
    auto r = app.parse(argv.argc(), argv.argv());
    REQUIRE(r.is_ok());
    // CLI > default: occurrences are the value (2), not default + 2 (7).
    CHECK(r.unwrap().get<int, fixed_string("v")>() == 2);
}

TEST_CASE("CountOption default_value_str and has_default")
{
    App app("test", "1.0", "Count default str");
    app.option<fixed_string("v")>("--verbose", 'v', "Verbosity").count().default_value(3);
    auto *def = app.find_option_by_long("verbose");
    REQUIRE(def != nullptr);
    CHECK(def->has_default());
    CHECK(def->default_value_str() == "3");
}

TEST_CASE("CountOption no default keeps has_default false")
{
    App app("test", "1.0", "Count no default");
    app.option<fixed_string("v")>("--verbose", 'v', "Verbosity").count();
    auto *def = app.find_option_by_long("verbose");
    REQUIRE(def != nullptr);
    CHECK_FALSE(def->has_default());
    CHECK(def->default_value_str().empty());
}

TEST_CASE("CountOption has_value false")
{
    App app("test", "1.0", "Has value");
    app.option<fixed_string("v")>("--verbose", 'v', "Verbosity").count();
    auto &cmd = app;
    // CountOption should report has_value=false
    auto *def = cmd.find_option_by_long("verbose");
    CHECK(def != nullptr);
    CHECK_FALSE(def->has_value());
}
