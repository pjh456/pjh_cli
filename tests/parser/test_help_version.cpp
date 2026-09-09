#include <doctest/doctest.h>

#include <functional>
#include <pjh_cli/app.hpp>
#include <pjh_cli/command/leaf_command.hpp>
#include <pjh_cli/core/fixed_string.hpp>
#include <pjh_cli/parse/parser.hpp>
#include <string>
#include <string_view>
#include <vector>

#include "test_helpers.hpp"

TEST_CASE("Parser --help sets help_requested")
{
    App app("test", "1.0", "Help test");
    app.option<fixed_string("verbose")>("--verbose", "Verbose").boolean();
    Argv argv{"test", "--help"};
    auto r = app.parse(argv.argc(), argv.argv());
    REQUIRE(r.is_ok());
    auto &ctx = r.unwrap();
    CHECK(ctx.help_requested());
    CHECK_FALSE(ctx.version_requested());
    auto help = ctx.help_text();
    CHECK(help.find("Usage:") != std::string::npos);
    CHECK(help.find("Help test") != std::string::npos);
    CHECK(help.find("--verbose") != std::string::npos);
}

TEST_CASE("Parser -h sets help_requested")
{
    App app("test", "1.0", "Short help test");
    app.option<fixed_string("verbose")>("--verbose", "Verbose").boolean();
    Argv argv{"test", "-h"};
    auto r = app.parse(argv.argc(), argv.argv());
    REQUIRE(r.is_ok());
    auto &ctx = r.unwrap();
    CHECK(ctx.help_requested());
    CHECK_FALSE(ctx.version_requested());
}

TEST_CASE("Parser --version sets version_requested")
{
    App app("test", "1.0", "Version test");
    Argv argv{"test", "--version"};
    auto r = app.parse(argv.argc(), argv.argv());
    REQUIRE(r.is_ok());
    auto &ctx = r.unwrap();
    CHECK(ctx.version_requested());
    CHECK_FALSE(ctx.help_requested());
    CHECK(ctx.version_text() == "test version 1.0\n");
}

TEST_CASE("Parser subcommand --help targets subcommand")
{
    App app("test", "1.0", "Subcommand help test");
    app.add_leaf("serve", "Start server");
    Argv argv{"test", "serve", "--help"};
    auto r = app.parse(argv.argc(), argv.argv());
    REQUIRE(r.is_ok());
    auto &ctx = r.unwrap();
    CHECK(ctx.help_requested());
    auto help = ctx.help_text();
    CHECK(help.find("serve") != std::string::npos);
    CHECK(help.find("Start server") != std::string::npos);
    CHECK(help.starts_with("Usage: test serve"));
}

TEST_CASE("Parser nested subcommand --help shows full path")
{
    App app("test", "1.0", "Nested help test");
    auto &container = app.add_branch("container", "Container");
    container.add_leaf("start", "Start");
    Argv argv{"test", "container", "start", "--help"};
    auto r = app.parse(argv.argc(), argv.argv());
    REQUIRE(r.is_ok());
    auto help = r.unwrap().help_text();
    CHECK(help.starts_with("Usage: test container start"));
}

TEST_CASE("Parser alias --help shows canonical command path")
{
    App app("test", "1.0", "Alias help test");
    app.add_leaf("server", "Server").alias("srv");
    Argv argv{"test", "srv", "--help"};
    auto r = app.parse(argv.argc(), argv.argv());
    REQUIRE(r.is_ok());
    CHECK(r.unwrap().help_text().starts_with("Usage: test server"));
}

TEST_CASE("Parser --help skips finalization")
{
    App app("test", "1.0", "Skip finalization test");
    app.option<fixed_string("port")>("--port", "Port", 8080);
    Argv argv{"test", "--help"};
    auto r = app.parse(argv.argc(), argv.argv());
    REQUIRE(r.is_ok());
    auto &ctx = r.unwrap();
    CHECK(ctx.help_requested());
    CHECK_FALSE(ctx.has<fixed_string("port")>());
}

TEST_CASE("Parser --help bypasses required checks")
{
    App app("test", "1.0", "Required bypass test");
    app.option<fixed_string("port")>("--port", "Port").integer().required();
    Argv argv{"test", "--help"};
    auto r = app.parse(argv.argc(), argv.argv());
    REQUIRE(r.is_ok());
    CHECK(r.unwrap().help_requested());
}

TEST_CASE("Parser --help shows option metadata annotations")
{
    App app("test", "1.0", "Metadata help test");
    app.option<fixed_string("host")>("--host", "Host").str().env("APP_HOST");
    app.option<fixed_string("compress")>("--compress", "Compress").boolean().negatable();
    Argv argv{"test", "--help"};
    auto r = app.parse(argv.argc(), argv.argv());
    REQUIRE(r.is_ok());
    auto help = r.unwrap().help_text();
    CHECK(help.find("(env: APP_HOST)") != std::string::npos);
    CHECK(help.find("(negatable)") != std::string::npos);
}

TEST_CASE("Parser --help after double dash is not intercepted")
{
    App app("test", "1.0", "Double dash help test");
    Argv argv{"test", "--", "--help"};
    auto r = app.parse(argv.argc(), argv.argv());
    REQUIRE(r.is_ok());
    CHECK_FALSE(r.unwrap().help_requested());
}

TEST_CASE("Parser --version after double dash is not intercepted")
{
    App app("test", "1.0", "Double dash version test");
    Argv argv{"test", "--", "--version"};
    auto r = app.parse(argv.argc(), argv.argv());
    REQUIRE(r.is_ok());
    CHECK_FALSE(r.unwrap().version_requested());
}

TEST_CASE("Parser parse_fuzzy --help sets help_requested")
{
    App app("test", "1.0", "Fuzzy help test");
    Argv argv{"test", "--help"};
    auto r = app.parse_fuzzy(argv.argc(), argv.argv());
    REQUIRE(r.is_ok());
    CHECK(r.unwrap().help_requested());
}

TEST_CASE("Parser --help uses injected formatter")
{
    App app("test", "1.0", "Injected help test");
    app.add_leaf("serve", "Start server");
    std::string observed_name;
    HelpFormatterFn fmt = [&observed_name](const BaseCommand &cmd)
    {
        observed_name = cmd.name();
        return std::string("CUSTOM HELP");
    };
    Argv argv{"test", "serve", "--help"};
    auto r = Parser::parse_command(app, argv.argc(), argv.argv(), 0, fmt);
    REQUIRE(r.is_ok());
    auto &ctx = r.unwrap();
    CHECK(ctx.help_requested());
    CHECK(ctx.help_text() == "CUSTOM HELP");
    CHECK(observed_name == "serve");
}

TEST_CASE("App --help uses injected formatter")
{
    App app("test", "1.0", "App injected help test");
    app.set_help_formatter([](const BaseCommand &) { return std::string("APP CUSTOM"); });
    Argv argv{"test", "--help"};
    auto r = app.parse(argv.argc(), argv.argv());
    REQUIRE(r.is_ok());
    CHECK(r.unwrap().help_text() == "APP CUSTOM");
}

TEST_CASE("App parse_fuzzy --help uses injected formatter")
{
    App app("test", "1.0", "Fuzzy injected help test");
    app.set_help_formatter([](const BaseCommand &)
                           { return std::string("FUZZY CUSTOM"); });
    Argv argv{"test", "--help"};
    auto r = app.parse_fuzzy(argv.argc(), argv.argv());
    REQUIRE(r.is_ok());
    CHECK(r.unwrap().help_text() == "FUZZY CUSTOM");
}

TEST_CASE("Injected formatter receives the requested command")
{
    App app("test", "1.0", "Command observation test");
    auto &container = app.add_branch("container", "Container");
    container.add_leaf("start", "Start");
    std::vector<std::string> seen;
    HelpFormatterFn fmt = [&seen](const BaseCommand &cmd)
    {
        seen.push_back(cmd.name());
        return std::string("x");
    };

    {
        Argv argv{"test", "--help"};
        auto r = Parser::parse_command(app, argv.argc(), argv.argv(), 0, fmt);
        REQUIRE(r.is_ok());
    }
    {
        Argv argv{"test", "container", "start", "--help"};
        auto r = Parser::parse_command(app, argv.argc(), argv.argv(), 0, fmt);
        REQUIRE(r.is_ok());
    }

    REQUIRE(seen.size() == 2);
    CHECK(seen[0] == "test");
    CHECK(seen[1] == "start");
}

TEST_CASE("Injected formatter is not called without a help request")
{
    App app("test", "1.0", "No help call test");
    app.add_leaf("serve", "Start server");
    int calls = 0;
    HelpFormatterFn fmt = [&calls](const BaseCommand &)
    {
        calls++;
        return std::string("x");
    };

    {
        Argv argv{"test"};
        auto r = Parser::parse_command(app, argv.argc(), argv.argv(), 0, fmt);
        REQUIRE(r.is_ok());
    }
    {
        Argv argv{"test", "serve"};
        auto r = Parser::parse_command(app, argv.argc(), argv.argv(), 0, fmt);
        REQUIRE(r.is_ok());
    }
    CHECK(calls == 0);
}

TEST_CASE("Injected formatter is not called after double dash")
{
    App app("test", "1.0", "Double dash injection test");
    int calls = 0;
    HelpFormatterFn fmt = [&calls](const BaseCommand &)
    {
        calls++;
        return std::string("x");
    };
    Argv argv{"test", "--", "--help"};
    auto r = Parser::parse_command(app, argv.argc(), argv.argv(), 0, fmt);
    REQUIRE(r.is_ok());
    CHECK_FALSE(r.unwrap().help_requested());
    CHECK(calls == 0);
}

TEST_CASE("Default help formatter used when none injected")
{
    App app("test", "1.0", "Default seam test");
    Argv argv{"test", "--help"};
    auto r = Parser::parse_command(app, argv.argc(), argv.argv());
    REQUIRE(r.is_ok());
    CHECK(r.unwrap().help_text().starts_with("Usage: test"));
}

TEST_CASE("Clearing the help formatter restores the built-in")
{
    App app("test", "1.0", "Clear formatter test");
    app.set_help_formatter([](const BaseCommand &) { return std::string("CUSTOM"); });
    Argv argv{"test", "--help"};
    {
        auto r = app.parse(argv.argc(), argv.argv());
        REQUIRE(r.is_ok());
        CHECK(r.unwrap().help_text() == "CUSTOM");
    }
    app.set_help_formatter({});
    CHECK_FALSE(app.help_formatter());
    {
        auto r = app.parse(argv.argc(), argv.argv());
        REQUIRE(r.is_ok());
        CHECK(r.unwrap().help_text().starts_with("Usage: test"));
    }
}

TEST_CASE("Injected help formatter does not affect --version")
{
    App app("test", "1.0", "Version injection test");
    int calls = 0;
    app.set_help_formatter(
        [&calls](const BaseCommand &)
        {
            calls++;
            return std::string("x");
        });
    Argv argv{"test", "--version"};
    auto r = app.parse(argv.argc(), argv.argv());
    REQUIRE(r.is_ok());
    auto &ctx = r.unwrap();
    CHECK(ctx.version_text() == "test version 1.0\n");
    CHECK(calls == 0);
}

TEST_CASE("App help_formatter is empty by default")
{
    App app("test", "1.0", "Default formatter state test");
    CHECK_FALSE(app.help_formatter());
}

TEST_CASE("Empty injected help text cancels help_requested")
{
    App app("test", "1.0", "Empty help contract test");
    app.set_help_formatter([](const BaseCommand &) { return std::string{}; });
    Argv argv{"test", "--help"};
    auto r = app.parse(argv.argc(), argv.argv());
    REQUIRE(r.is_ok());
    auto &ctx = r.unwrap();
    CHECK_FALSE(ctx.help_requested());
    CHECK(ctx.help_text().empty());
}
