#include <doctest/doctest.h>

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

// ── choices ──

TEST_CASE("StrOption choices accepts valid value")
{
    App app("test", "1.0", "Choices ok");
    app.option<fixed_string("fmt")>("--format", 'f', "Format")
        .str()
        .choices({"json", "yaml", "toml"});
    Argv argv{"test", "--format", "json"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_ok());
    CHECK(r.unwrap().get<std::string, fixed_string("fmt")>() == "json");
}

TEST_CASE("StrOption choices rejects invalid value")
{
    App app("test", "1.0", "Choices reject");
    app.option<fixed_string("fmt")>("--format", 'f', "Format")
        .str()
        .choices({"json", "yaml"});
    Argv argv{"test", "--format", "toml"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_err());
}

TEST_CASE("StrOption choices case-sensitive")
{
    App app("test", "1.0", "Choices case");
    app.option<fixed_string("fmt")>("--format", 'f', "Format")
        .str()
        .choices({"json", "yaml"});
    Argv argv{"test", "--format", "JSON"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_err());
}

TEST_CASE("StrOption choices error message")
{
    App app("test", "1.0", "Choices msg");
    app.option<fixed_string("fmt")>("--format", 'f', "Format")
        .str()
        .choices({"json", "yaml", "toml"});
    Argv argv{"test", "--format", "xml"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_err());
    CHECK(
        r.unwrap_err().what() == std::string_view(
                                     "Parse Error: invalid value 'xml' for 'format': "
                                     "expected one of: json, yaml, toml"));
}

TEST_CASE("StrOption choices without choices still accepts anything")
{
    App app("test", "1.0", "Choices none");
    app.option<fixed_string("name")>("--name", "Name").str();
    Argv argv{"test", "--name", "anything"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_ok());
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

TEST_CASE("StrOption repeatable with choices validates each value")
{
    App app("test", "1.0", "Repeat choices");
    app.option<fixed_string("fmt")>("--format", 'f', "Format")
        .str()
        .choices({"json", "yaml"})
        .repeatable();
    Argv argv{"test", "-f", "json", "-f", "yaml"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_ok());
    Argv argv2{"test", "-f", "json", "-f", "xml"};
    CHECK(app.parse(argv2.argc(), argv2.argv()).is_err());
}
