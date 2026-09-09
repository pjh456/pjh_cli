#include <doctest/doctest.h>

#include <pjh_cli/app.hpp>
#include <pjh_cli/command/leaf_command.hpp>
#include <pjh_cli/core/fixed_string.hpp>

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
