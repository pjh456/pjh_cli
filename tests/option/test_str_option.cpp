#include <doctest/doctest.h>

#include <pjh_cli.hpp>
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
