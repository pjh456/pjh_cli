#include <doctest/doctest.h>

#include <iostream>
#include <initializer_list>
#include <pjh_cli/app.hpp>
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

TEST_CASE("StrOption parse_value stores string")
{
    App app("test", "1.0", "Str");
    app.option<fixed_string("name")>("--name", "Name").str();
    Argv argv{"test", "--name", "hello"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_ok());
    CHECK(r.unwrap().get<std::string, fixed_string("name")>() == "hello");
}

TEST_CASE("StrOption default_value applied when absent")
{
    App app("test", "1.0", "Str default");
    app.option<fixed_string("name")>("--name", "Name").str().default_value("world");
    Argv argv{"test"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_ok());
    CHECK(r.unwrap().get<std::string, fixed_string("name")>() == "world");
}

TEST_CASE("StrOption default_value overridden by CLI")
{
    App app("test", "1.0", "Str override");
    app.option<fixed_string("name")>("--name", "Name").str().default_value("world");
    Argv argv{"test", "--name", "hello"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_ok());
    CHECK(r.unwrap().get<std::string, fixed_string("name")>() == "hello");
}

TEST_CASE("StrOption has_default reflects default_value")
{
    App app("test", "1.0", "Str has_default");
    app.option<fixed_string("name")>("--name", "Name").str().default_value("world");
    auto *def = app.find_option_by_long("name");
    CHECK(def != nullptr);
    CHECK(def->has_default());
}

// ── repeatable ──

TEST_CASE("StrOption repeatable appends multiple values")
{
    App app("test", "1.0", "Repeat str");
    app.option<fixed_string("inc")>("--include", 'I', "Include").str().repeatable();
    Argv argv{"test", "-I", "/a", "-I", "/b", "-I", "/c"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_ok());
    auto all = r.unwrap().get_all<std::string, fixed_string("inc")>();
    REQUIRE(all.size() == 3);
    CHECK(all[0] == "/a");
    CHECK(all[1] == "/b");
    CHECK(all[2] == "/c");
}

TEST_CASE("StrOption repeatable single value still works")
{
    App app("test", "1.0", "Repeat str one");
    app.option<fixed_string("inc")>("--include", 'I', "Include").str().repeatable();
    Argv argv{"test", "-I", "/a"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_ok());
    auto all = r.unwrap().get_all<std::string, fixed_string("inc")>();
    REQUIRE(all.size() == 1);
    CHECK(all[0] == "/a");
}
