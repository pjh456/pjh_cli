#include <doctest/doctest.h>

#include <pjh_cli/app.hpp>
#include <pjh_cli/core/fixed_string.hpp>
#include <pjh_cli/parse/parse_context.hpp>
#include <string>

#include "../option/test_helpers.hpp"
#include "test_helpers.hpp"

namespace
{
    /// @brief Parse @p argv against a root `--port`/`-p` int option and report
    ///        whether the option was explicitly provided on the command line.
    bool port_was_provided(Argv &argv)
    {
        App app("test", "1.0", "value syntaxes");
        app.option<fixed_string("port")>("--port", 'p', "Port").integer();
        auto r = app.parse(argv.argc(), argv.argv());
        return r.is_ok() && r.unwrap().was_provided<fixed_string("port")>();
    }
}  // namespace

TEST_CASE("was_provided is true for a CLI option")
{
    App app("test", "1.0", "was_provided CLI");
    app.option<fixed_string("port")>("--port", "Port").integer();
    Argv argv{"test", "--port", "8080"};
    auto r = app.parse(argv.argc(), argv.argv());
    REQUIRE(r.is_ok());
    auto &ctx = r.unwrap();
    CHECK(ctx.was_provided<fixed_string("port")>());
    CHECK(ctx.has<fixed_string("port")>());
    CHECK(ctx.get<int, fixed_string("port")>() == 8080);
}

TEST_CASE("was_provided is false for a default")
{
    App app("test", "1.0", "was_provided default");
    app.option<fixed_string("port")>("--port", "Port").integer().default_value(8080);
    Argv argv{"test"};
    auto r = app.parse(argv.argc(), argv.argv());
    REQUIRE(r.is_ok());
    auto &ctx = r.unwrap();
    CHECK(ctx.has<fixed_string("port")>());
    CHECK(ctx.get<int, fixed_string("port")>() == 8080);
    CHECK_FALSE(ctx.was_provided<fixed_string("port")>());
}

TEST_CASE("was_provided is false for env fallback")
{
    ScopedEnvVar env_guard{"PJH_CLI_TEST_WAS_PROVIDED_PORT", "8080"};
    App app("test", "1.0", "was_provided env");
    app.option<fixed_string("port")>("--port", "Port").integer().env(env_guard.name());
    Argv argv{"test"};
    auto r = app.parse(argv.argc(), argv.argv());
    REQUIRE(r.is_ok());
    auto &ctx = r.unwrap();
    CHECK(ctx.has<fixed_string("port")>());
    CHECK(ctx.get<int, fixed_string("port")>() == 8080);
    CHECK_FALSE(ctx.was_provided<fixed_string("port")>());
}

TEST_CASE("was_provided is true when the CLI overrides env")
{
    ScopedEnvVar env_guard{"PJH_CLI_TEST_WAS_PROVIDED_OVERRIDE", "3000"};
    App app("test", "1.0", "was_provided CLI env");
    app.option<fixed_string("port")>("--port", "Port").integer().env(env_guard.name());
    Argv argv{"test", "--port", "8080"};
    auto r = app.parse(argv.argc(), argv.argv());
    REQUIRE(r.is_ok());
    auto &ctx = r.unwrap();
    CHECK(ctx.get<int, fixed_string("port")>() == 8080);
    CHECK(ctx.was_provided<fixed_string("port")>());
}

TEST_CASE("was_provided is false for env over default")
{
    ScopedEnvVar env_guard{"PJH_CLI_TEST_WAS_PROVIDED_ENVDEFAULT", "3000"};
    App app("test", "1.0", "was_provided env over default");
    app.option<fixed_string("port")>("--port", "Port")
        .integer()
        .default_value(5000)
        .env(env_guard.name());
    Argv argv{"test"};
    auto r = app.parse(argv.argc(), argv.argv());
    REQUIRE(r.is_ok());
    auto &ctx = r.unwrap();
    CHECK(ctx.has<fixed_string("port")>());
    CHECK(ctx.get<int, fixed_string("port")>() == 3000);
    CHECK_FALSE(ctx.was_provided<fixed_string("port")>());
}

TEST_CASE("was_provided covers negation")
{
    App app("test", "1.0", "was_provided negation");
    app.option<fixed_string("verbose")>("--verbose", 'v', "Verbose")
        .boolean()
        .default_value(true)
        .negatable();

    {
        Argv argv{"test"};
        auto r = app.parse(argv.argc(), argv.argv());
        REQUIRE(r.is_ok());
        auto &ctx = r.unwrap();
        CHECK(ctx.has<fixed_string("verbose")>());
        CHECK(ctx.get<bool, fixed_string("verbose")>() == true);
        CHECK_FALSE(ctx.was_provided<fixed_string("verbose")>());
    }
    {
        Argv argv{"test", "--no-verbose"};
        auto r = app.parse(argv.argc(), argv.argv());
        REQUIRE(r.is_ok());
        auto &ctx = r.unwrap();
        CHECK(ctx.get<bool, fixed_string("verbose")>() == false);
        CHECK(ctx.was_provided<fixed_string("verbose")>());
    }
}

TEST_CASE("was_provided covers count")
{
    App app("test", "1.0", "was_provided count");
    app.option<fixed_string("v")>("--verbose", 'v', "Verbosity").count();

    {
        Argv argv{"test", "-vvv"};
        auto r = app.parse(argv.argc(), argv.argv());
        REQUIRE(r.is_ok());
        auto &ctx = r.unwrap();
        CHECK(ctx.get<int, fixed_string("v")>() == 3);
        CHECK(ctx.was_provided<fixed_string("v")>());
    }
    {
        Argv argv{"test"};
        auto r = app.parse(argv.argc(), argv.argv());
        REQUIRE(r.is_ok());
        auto &ctx = r.unwrap();
        CHECK_FALSE(ctx.has<fixed_string("v")>());
        CHECK_FALSE(ctx.was_provided<fixed_string("v")>());
    }
}

TEST_CASE("was_provided covers repeatable")
{
    App app("test", "1.0", "was_provided repeatable");
    app.option<fixed_string("tag")>("--tag", 't', "Tag")
        .str()
        .repeatable()
        .default_value(std::string("fallback"));

    {
        Argv argv{"test", "--tag", "a", "--tag", "b"};
        auto r = app.parse(argv.argc(), argv.argv());
        REQUIRE(r.is_ok());
        auto &ctx = r.unwrap();
        auto all = ctx.get_all<std::string, fixed_string("tag")>();
        REQUIRE(all.size() == 2);
        CHECK(all[0] == "a");
        CHECK(all[1] == "b");
        CHECK(ctx.was_provided<fixed_string("tag")>());
    }
    {
        Argv argv{"test"};
        auto r = app.parse(argv.argc(), argv.argv());
        REQUIRE(r.is_ok());
        auto &ctx = r.unwrap();
        REQUIRE(ctx.has<fixed_string("tag")>());
        auto all = ctx.get_all<std::string, fixed_string("tag")>();
        REQUIRE(all.size() == 1);
        CHECK(all[0] == "fallback");
        CHECK_FALSE(ctx.was_provided<fixed_string("tag")>());
    }
}

TEST_CASE("was_provided covers grouped short options")
{
    App app("test", "1.0", "was_provided grouped");
    app.option<fixed_string("alpha")>("--alpha", 'a', "Alpha").boolean();
    app.option<fixed_string("beta")>("--beta", 'b', "Beta").boolean();
    Argv argv{"test", "-ab"};
    auto r = app.parse(argv.argc(), argv.argv());
    REQUIRE(r.is_ok());
    auto &ctx = r.unwrap();
    CHECK(ctx.get<bool, fixed_string("alpha")>() == true);
    CHECK(ctx.get<bool, fixed_string("beta")>() == true);
    CHECK(ctx.was_provided<fixed_string("alpha")>());
    CHECK(ctx.was_provided<fixed_string("beta")>());
}

TEST_CASE("was_provided covers value syntaxes")
{
    Argv separated{"test", "-p", "8080"};
    Argv compact{"test", "-p8080"};
    Argv compact_equals{"test", "-p=8080"};
    Argv long_equals{"test", "--port=8080"};

    CHECK(port_was_provided(separated));
    CHECK(port_was_provided(compact));
    CHECK(port_was_provided(compact_equals));
    CHECK(port_was_provided(long_equals));
}

TEST_CASE("was_provided follows the parent chain")
{
    {
        App app("test", "1.0", "was_provided parent before");
        app.option<fixed_string("verbose")>("--verbose", 'v', "Verbose").boolean();
        app.add_leaf("son", "Son");
        Argv argv{"test", "--verbose", "son"};
        auto r = app.parse(argv.argc(), argv.argv());
        REQUIRE(r.is_ok());
        CHECK(r.unwrap().was_provided<fixed_string("verbose")>());
    }
    {
        App app("test", "1.0", "was_provided parent after");
        app.option<fixed_string("verbose")>("--verbose", 'v', "Verbose").boolean();
        app.add_leaf("son", "Son");
        Argv argv{"test", "son", "--verbose"};
        auto r = app.parse(argv.argc(), argv.argv());
        REQUIRE(r.is_ok());
        CHECK(r.unwrap().was_provided<fixed_string("verbose")>());
    }
    {
        App app("test", "1.0", "was_provided parent default");
        app.option<fixed_string("verbose")>("--verbose", 'v', "Verbose")
            .boolean()
            .default_value(false);
        app.add_leaf("son", "Son");
        Argv argv{"test", "son"};
        auto r = app.parse(argv.argc(), argv.argv());
        REQUIRE(r.is_ok());
        auto &ctx = r.unwrap();
        CHECK(ctx.has<fixed_string("verbose")>());
        CHECK_FALSE(ctx.was_provided<fixed_string("verbose")>());
    }
}

TEST_CASE("was_provided covers positional args")
{
    App app("test", "1.0", "was_provided positional");
    auto &son = app.add_leaf("son", "Son");
    son.arg<int, 0>("count", "Count");

    {
        Argv argv{"test", "son", "42"};
        auto r = app.parse(argv.argc(), argv.argv());
        REQUIRE(r.is_ok());
        auto &ctx = r.unwrap();
        CHECK(ctx.has<0>());
        CHECK(ctx.get<int, 0>() == 42);
        CHECK(ctx.was_provided<0>());
    }
    {
        Argv argv{"test", "son"};
        auto r = app.parse(argv.argc(), argv.argv());
        REQUIRE(r.is_ok());
        auto &ctx = r.unwrap();
        CHECK_FALSE(ctx.has<0>());
        CHECK_FALSE(ctx.was_provided<0>());
    }
}

TEST_CASE("was_provided is orthogonal to has")
{
    ScopedEnvVar env_guard{"PJH_CLI_TEST_WAS_PROVIDED_ORTHO", "30"};
    App app("test", "1.0", "was_provided orthogonal");
    app.option<fixed_string("cli")>("--cli", "Cli").integer();
    app.option<fixed_string("env")>("--env", "Env").integer().env(env_guard.name());
    app.option<fixed_string("def")>("--def", "Def").integer().default_value(7);
    Argv argv{"test", "--cli", "1"};
    auto r = app.parse(argv.argc(), argv.argv());
    REQUIRE(r.is_ok());
    auto &ctx = r.unwrap();

    CHECK(ctx.has<fixed_string("cli")>());
    CHECK(ctx.was_provided<fixed_string("cli")>());

    CHECK(ctx.has<fixed_string("env")>());
    CHECK_FALSE(ctx.was_provided<fixed_string("env")>());

    CHECK(ctx.has<fixed_string("def")>());
    CHECK_FALSE(ctx.was_provided<fixed_string("def")>());
}

TEST_CASE("Nearest declaration env wins across the command chain")
{
    ScopedEnvVar root_env{"PJH_CLI_TEST_CHAIN_ROOT", "1"};
    ScopedEnvVar leaf_env{"PJH_CLI_TEST_CHAIN_LEAF", "2"};
    App app("test", "1.0", "Nearest env wins");
    app.option<fixed_string("port")>("--port", "Port").integer().env(root_env.name());
    auto &son = app.add_leaf("son", "Son");
    son.option<fixed_string("port")>("--port", "Port").integer().env(leaf_env.name());
    Argv argv{"test", "son"};
    auto r = app.parse(argv.argc(), argv.argv());
    REQUIRE(r.is_ok());
    auto &ctx = r.unwrap();
    CHECK(ctx.has<fixed_string("port")>());
    CHECK(ctx.get<int, fixed_string("port")>() == 2);
    CHECK_FALSE(ctx.was_provided<fixed_string("port")>());
}

TEST_CASE("Ancestor env applies when the near declaration has no env")
{
    ScopedEnvVar root_env{"PJH_CLI_TEST_CHAIN_FALLBACK", "1"};
    App app("test", "1.0", "Ancestor env fallback");
    app.option<fixed_string("port")>("--port", "Port").integer().env(root_env.name());
    auto &son = app.add_leaf("son", "Son");
    son.option<fixed_string("port")>("--port", "Port").integer();
    Argv argv{"test", "son"};
    auto r = app.parse(argv.argc(), argv.argv());
    REQUIRE(r.is_ok());
    auto &ctx = r.unwrap();
    CHECK(ctx.has<fixed_string("port")>());
    CHECK(ctx.get<int, fixed_string("port")>() == 1);
    CHECK_FALSE(ctx.was_provided<fixed_string("port")>());
}

TEST_CASE("Env beats default across declarations")
{
    ScopedEnvVar root_env{"PJH_CLI_TEST_CHAIN_ENVDEFAULT", "1"};
    App app("test", "1.0", "Env beats default across");
    app.option<fixed_string("port")>("--port", "Port").integer().env(root_env.name());
    auto &son = app.add_leaf("son", "Son");
    son.option<fixed_string("port")>("--port", "Port").integer().default_value(9);
    Argv argv{"test", "son"};
    auto r = app.parse(argv.argc(), argv.argv());
    REQUIRE(r.is_ok());
    auto &ctx = r.unwrap();
    CHECK(ctx.has<fixed_string("port")>());
    CHECK(ctx.get<int, fixed_string("port")>() == 1);
    CHECK_FALSE(ctx.was_provided<fixed_string("port")>());
}

TEST_CASE("Cross-chain default is not was_provided")
{
    App app("test", "1.0", "Cross-chain default not provided");
    app.option<fixed_string("port")>("--port", "Port").integer().default_value(1);
    auto &son = app.add_leaf("son", "Son");
    son.option<fixed_string("port")>("--port", "Port").integer().default_value(2);
    Argv argv{"test", "son"};
    auto r = app.parse(argv.argc(), argv.argv());
    REQUIRE(r.is_ok());
    auto &ctx = r.unwrap();
    CHECK(ctx.has<fixed_string("port")>());
    CHECK(ctx.get<int, fixed_string("port")>() == 2);
    CHECK_FALSE(ctx.was_provided<fixed_string("port")>());
}
