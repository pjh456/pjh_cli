#include <doctest/doctest.h>

#include <cstdlib>
#include <iostream>

#ifdef _WIN32
#define setenv(name, val, overwrite) _putenv_s(name, val)
#define unsetenv(name) _putenv_s(name, "")
#endif

#include <initializer_list>
#include <pjh_cli/app.hpp>
#include <pjh_cli/core/fixed_string.hpp>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

using namespace pjh::cli;

static_assert(
    !noexcept(std::declval<const detail::EnvSnapshot &>().get(std::string_view{})),
    "EnvSnapshot::get allocates a lookup key; must not be noexcept");

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

TEST_CASE("EnvVar int option reads from environment")
{
    setenv("TEST_PORT", "8080", 1);
    App app("test", "1.0", "Env int");
    app.option<fixed_string("port")>("--port", "Port").integer().env("TEST_PORT");
    Argv argv{"test"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_ok());
    CHECK(r.unwrap().get<int, fixed_string("port")>() == 8080);
    unsetenv("TEST_PORT");
}

TEST_CASE("EnvVar string option reads from environment")
{
    setenv("TEST_HOST", "example.com", 1);
    App app("test", "1.0", "Env str");
    app.option<fixed_string("host")>("--host", "Host").str().env("TEST_HOST");
    Argv argv{"test"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_ok());
    CHECK(r.unwrap().get<std::string, fixed_string("host")>() == "example.com");
    unsetenv("TEST_HOST");
}

TEST_CASE("EnvVar CLI value overrides environment")
{
    setenv("TEST_PORT", "3000", 1);
    App app("test", "1.0", "Env override");
    app.option<fixed_string("port")>("--port", "Port").integer().env("TEST_PORT");
    Argv argv{"test", "--port", "8080"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_ok());
    CHECK(r.unwrap().get<int, fixed_string("port")>() == 8080);
    unsetenv("TEST_PORT");
}

TEST_CASE("EnvVar default falls through to hardcoded default")
{
    unsetenv("TEST_PORT");
    App app("test", "1.0", "Env default");
    app.option<fixed_string("port")>("--port", "Port")
        .integer()
        .default_value(9999)
        .env("TEST_PORT");
    Argv argv{"test"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_ok());
    CHECK(r.unwrap().get<int, fixed_string("port")>() == 9999);
}

TEST_CASE("EnvVar satisfies required")
{
    setenv("TEST_TOKEN", "secret123", 1);
    App app("test", "1.0", "Env required");
    app.option<fixed_string("token")>("--token", "Token")
        .str()
        .env("TEST_TOKEN")
        .required();
    Argv argv{"test"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_ok());
    CHECK(r.unwrap().get<std::string, fixed_string("token")>() == "secret123");
    unsetenv("TEST_TOKEN");
}

TEST_CASE("EnvVar absent env var does not set value")
{
    unsetenv("TEST_PORT");
    App app("test", "1.0", "Env absent");
    app.option<fixed_string("port")>("--port", "Port").integer().env("TEST_PORT");
    Argv argv{"test"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_ok());
    CHECK_FALSE(r.unwrap().has<fixed_string("port")>());
}

TEST_CASE("EnvVar env overrides default when both set")
{
    setenv("TEST_PORT", "3000", 1);
    App app("test", "1.0", "Env over default");
    app.option<fixed_string("port")>("--port", "Port")
        .integer()
        .default_value(5000)
        .env("TEST_PORT");
    Argv argv{"test"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_ok());
    CHECK(r.unwrap().get<int, fixed_string("port")>() == 3000);
    unsetenv("TEST_PORT");
}

TEST_CASE("EnvVar bool option reads true from environment")
{
    setenv("TEST_VERBOSE", "true", 1);
    App app("test", "1.0", "Env bool true");
    app.option<fixed_string("verbose")>("--verbose", "Verbose")
        .boolean()
        .env("TEST_VERBOSE");
    Argv argv{"test"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_ok());
    CHECK(r.unwrap().get<bool, fixed_string("verbose")>() == true);
    unsetenv("TEST_VERBOSE");
}

TEST_CASE("EnvVar bool option reads false from environment")
{
    setenv("TEST_VERBOSE", "0", 1);
    App app("test", "1.0", "Env bool false");
    app.option<fixed_string("verbose")>("--verbose", "Verbose")
        .boolean()
        .env("TEST_VERBOSE");
    Argv argv{"test"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_ok());
    CHECK(r.unwrap().get<bool, fixed_string("verbose")>() == false);
    unsetenv("TEST_VERBOSE");
}

TEST_CASE("EnvVar bool option overrides default")
{
    setenv("TEST_VERBOSE", "true", 1);
    App app("test", "1.0", "Env bool over default");
    app.option<fixed_string("verbose")>("--verbose", "Verbose")
        .boolean()
        .default_value(false)
        .env("TEST_VERBOSE");
    Argv argv{"test"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_ok());
    CHECK(r.unwrap().get<bool, fixed_string("verbose")>() == true);
    unsetenv("TEST_VERBOSE");
}

TEST_CASE("EnvVar bool CLI flag overrides environment")
{
    setenv("TEST_VERBOSE", "false", 1);
    App app("test", "1.0", "Env bool CLI override");
    app.option<fixed_string("verbose")>("--verbose", "Verbose")
        .boolean()
        .env("TEST_VERBOSE");
    Argv argv{"test", "--verbose"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_ok());
    CHECK(r.unwrap().get<bool, fixed_string("verbose")>() == true);
    unsetenv("TEST_VERBOSE");
}

TEST_CASE("EnvVar bool invalid value errors")
{
    setenv("TEST_VERBOSE", "maybe", 1);
    App app("test", "1.0", "Env bool invalid");
    app.option<fixed_string("verbose")>("--verbose", "Verbose")
        .boolean()
        .env("TEST_VERBOSE");
    Argv argv{"test"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_err());
    auto msg = std::string_view(r.unwrap_err().what());
    CHECK(msg.find("invalid value 'maybe' for '--verbose'") != std::string_view::npos);
    CHECK(msg.find("expected bool") != std::string_view::npos);
    unsetenv("TEST_VERBOSE");
}

TEST_CASE("EnvVar option on subcommand reads environment")
{
    setenv("TEST_HOST", "sub.example.com", 1);
    App app("test", "1.0", "Env subcommand");
    app.add_leaf("sub", "Sub")
        .option<fixed_string("host")>("--host", "Host")
        .str()
        .env("TEST_HOST");
    Argv argv{"test", "sub"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_ok());
    CHECK(r.unwrap().get<std::string, fixed_string("host")>() == "sub.example.com");
    unsetenv("TEST_HOST");
}

#ifdef _WIN32
TEST_CASE("EnvSnapshot reads Windows environment")
{
    _putenv_s("PJH_CLI_TEST_ENV", "42");
    detail::EnvSnapshot snap;
    auto *v = snap.get("PJH_CLI_TEST_ENV");
    REQUIRE(v != nullptr);
    CHECK(*v == "42");
    _putenv_s("PJH_CLI_TEST_ENV", "");
}
#endif
