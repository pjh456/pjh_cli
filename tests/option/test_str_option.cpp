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
