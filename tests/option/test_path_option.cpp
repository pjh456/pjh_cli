#include <doctest/doctest.h>

#include <filesystem>
#include <initializer_list>
#include <iostream>
#include <pjh_cli/app.hpp>
#include <pjh_cli/core/fixed_string.hpp>
#include <string>
#include <string_view>
#include <vector>

#include "../argv.hpp"

using namespace pjh::cli;

TEST_CASE("PathOption parse_value stores path")
{
    App app("test", "1.0", "Path");
    app.option<fixed_string("output")>("--output", "Output file").path();
    Argv argv{"test", "--output", "/tmp/out.txt"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_ok());
    CHECK(
        r.unwrap().get<std::filesystem::path, fixed_string("output")>() ==
        std::filesystem::path("/tmp/out.txt"));
}

TEST_CASE("PathOption default_value applied when absent")
{
    App app("test", "1.0", "Path default");
    app.option<fixed_string("output")>("--output", "Output file")
        .path()
        .default_value(std::filesystem::path("./out.txt"));
    Argv argv{"test"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_ok());
    CHECK(
        r.unwrap().get<std::filesystem::path, fixed_string("output")>() ==
        std::filesystem::path("./out.txt"));
}

TEST_CASE("PathOption default_value_str renders via BuiltinTraits")
{
    App app("test", "1.0", "Path default str");
    app.option<fixed_string("output")>("--output", "Output file")
        .path()
        .default_value(std::filesystem::path("./out.txt"));
    auto *def = app.find_option_by_long("output");
    REQUIRE(def != nullptr);
    CHECK(def->default_value_str() == "./out.txt");
}

TEST_CASE("PathOption has_default reflects default_value")
{
    App app("test", "1.0", "Path has_default");
    app.option<fixed_string("output")>("--output", "Output file")
        .path()
        .default_value(std::filesystem::path("/tmp/a.txt"));
    auto *def = app.find_option_by_long("output");
    CHECK(def != nullptr);
    CHECK(def->has_default());
}
