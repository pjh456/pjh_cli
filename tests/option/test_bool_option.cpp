#include <doctest/doctest.h>

#include <iostream>
#include <initializer_list>
#include <pjh_cli/app.hpp>
#include <pjh_cli/command/leaf_command.hpp>
#include <pjh_cli/core/fixed_string.hpp>
#include <string>
#include <string_view>
#include <vector>

using namespace pjh::cli;

struct Argv
{
    std::vector<std::string> storage;
    std::vector<char *> ptrs;
    Argv(std::initializer_list<std::string> list) : storage(list)
    {
        for (auto &s : storage) ptrs.push_back(s.data());
    }
    int argc() const { return static_cast<int>(ptrs.size()); }
    char **argv() { return ptrs.data(); }
};

TEST_CASE("BoolOption flag sets true when present")
{
    App app("test", "1.0", "Bool flag");
    app.option<fixed_string("verbose")>("--verbose", 'v', "Verbose").boolean();
    Argv argv{"test", "--verbose"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_ok());
    CHECK(r.unwrap().get<bool, fixed_string("verbose")>() == true);
}

TEST_CASE("BoolOption absent means no value")
{
    App app("test", "1.0", "Bool absent");
    app.option<fixed_string("verbose")>("--verbose", 'v', "Verbose").boolean();
    Argv argv{"test"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_ok());
    CHECK_FALSE(r.unwrap().has<fixed_string("verbose")>());
}

TEST_CASE("BoolOption short flag sets true")
{
    App app("test", "1.0", "Bool short");
    app.option<fixed_string("verbose")>("--verbose", 'v', "Verbose").boolean();
    Argv argv{"test", "-v"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_ok());
    CHECK(r.unwrap().get<bool, fixed_string("verbose")>() == true);
}

TEST_CASE("BoolOption grouped short flags all set")
{
    App app("test", "1.0", "Bool group");
    app.option<fixed_string("a")>("--a", 'a', "A").boolean();
    app.option<fixed_string("b")>("--b", 'b', "B").boolean();
    app.option<fixed_string("c")>("--c", 'c', "C").boolean();
    Argv argv{"test", "-abc"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_ok());
    auto &ctx = r.unwrap();
    CHECK(ctx.get<bool, fixed_string("a")>() == true);
    CHECK(ctx.get<bool, fixed_string("b")>() == true);
    CHECK(ctx.get<bool, fixed_string("c")>() == true);
}

TEST_CASE("BoolOption default_value applied when absent")
{
    App app("test", "1.0", "Bool default");
    app.option<fixed_string("verbose")>("--verbose", 'v', "Verbose")
        .boolean()
        .default_value(true);
    Argv argv{"test"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_ok());
    CHECK(r.unwrap().get<bool, fixed_string("verbose")>() == true);
}

TEST_CASE("BoolOption default_value overridden by CLI flag")
{
    App app("test", "1.0", "Bool override");
    app.option<fixed_string("verbose")>("--verbose", 'v', "Verbose")
        .boolean()
        .default_value(true);
    Argv argv{"test"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_ok());
    CHECK(r.unwrap().get<bool, fixed_string("verbose")>() == true);
}

TEST_CASE("BoolOption has_default reflects default_value")
{
    App app("test", "1.0", "Bool has_default");
    app.option<fixed_string("verbose")>("--verbose", 'v', "Verbose")
        .boolean()
        .default_value(false);
    auto *def = app.find_option_by_long("verbose");
    CHECK(def != nullptr);
    CHECK(def->has_default());
}

TEST_CASE("BoolOption has_value false")
{
    App app("test", "1.0", "Bool has_value");
    app.option<fixed_string("verbose")>("--verbose", 'v', "Verbose").boolean();
    auto *def = app.find_option_by_long("verbose");
    CHECK(def != nullptr);
    CHECK_FALSE(def->has_value());
}

// ── negatable ──

TEST_CASE("BoolOption negatable --no-xxx sets false")
{
    App app("test", "1.0", "Negatable");
    app.option<fixed_string("verbose")>("--verbose", 'v', "Verbose")
        .boolean()
        .negatable();
    Argv argv{"test", "--no-verbose"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_ok());
    CHECK(r.unwrap().get<bool, fixed_string("verbose")>() == false);
}

TEST_CASE("BoolOption negatable --verbose still sets true")
{
    App app("test", "1.0", "Negatable explicit");
    app.option<fixed_string("verbose")>("--verbose", 'v', "Verbose")
        .boolean()
        .negatable();
    Argv argv{"test", "--verbose"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_ok());
    CHECK(r.unwrap().get<bool, fixed_string("verbose")>() == true);
}

TEST_CASE("BoolOption negatable not negated returns error")
{
    App app("test", "1.0", "Not negatable");
    app.option<fixed_string("verbose")>("--verbose", 'v', "Verbose").boolean();
    // .negatable() NOT called → --no-verbose should error
    Argv argv{"test", "--no-verbose"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_err());
}

TEST_CASE("BoolOption negatable --no-verbose with extra =value ignored")
{
    App app("test", "1.0", "Neg eq");
    app.option<fixed_string("verbose")>("--verbose", 'v', "Verbose")
        .boolean()
        .negatable();
    Argv argv{"test", "--no-verbose=0"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_ok());
    CHECK(r.unwrap().get<bool, fixed_string("verbose")>() == false);
}

TEST_CASE("BoolOption negatable in subcommand")
{
    App app("test", "1.0", "Neg sub");
    auto &cmd = app.add_leaf("cmd", "Command");
    cmd.option<fixed_string("verbose")>("--verbose", 'v', "Verbose")
        .boolean()
        .negatable();
    Argv argv{"test", "cmd", "--no-verbose"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_ok());
    auto &ctx = r.unwrap();
    CHECK(ctx.matched_path() == "cmd");
    CHECK(ctx.get<bool, fixed_string("verbose")>() == false);
}
