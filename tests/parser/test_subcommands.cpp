#include <doctest/doctest.h>

#include <initializer_list>
#include <pjh_cli/app.hpp>
#include <pjh_cli/command/base_command.hpp>
#include <pjh_cli/core/fixed_string.hpp>
#include <pjh_cli/core/type.hpp>
#include <string>
#include <vector>

#include "test_helpers.hpp"

struct LocalArgv
{
    std::vector<std::string> storage;
    std::vector<char *> ptrs;
    LocalArgv(std::initializer_list<std::string> list) : storage(list)
    {
        for (auto &s : storage) ptrs.push_back(s.data());
    }
    int argc() const { return static_cast<int>(ptrs.size()); }
    char **argv() { return ptrs.data(); }
};

TEST_CASE("Parser subcommand matching")
{
    App app("test", "1.0", "Subcommand test");
    auto &serve = app.add_leaf("serve", "Start server");
    serve.option<fixed_string("port")>("--port", 'p', "Port").integer();
    Argv argv{"test", "serve", "--port", "8080"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_ok());
    auto &ctx = r.unwrap();
    CHECK(ctx.get<int, fixed_string("port")>() == 8080);
    CHECK(ctx.matched_path() == "serve");
}

TEST_CASE("Parser deep subcommand nesting")
{
    App app("test", "1.0", "Nested test");
    auto &db = app.add_branch("db", "Database commands");
    auto &migrate = db.add_leaf("migrate", "Run migrations");
    migrate.option<fixed_string("name")>("--name", 'n', "Migration name").str();
    Argv argv{"test", "db", "migrate", "--name", "v2"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_ok());
    auto &ctx = r.unwrap();
    CHECK(ctx.get<std::string, fixed_string("name")>() == "v2");
    CHECK(ctx.matched_path() == "db migrate");
}

TEST_CASE("Parser disabled subcommand skipped")
{
    App app("test", "1.0", "Disabled test");
    auto &active = app.add_leaf("active", "Available");
    active.option<fixed_string("x")>("--x", 'x', "Flag").boolean();
    app.add_leaf("disabled", "Unavailable").enabled([] { return false; });
    Argv argv{"test", "active", "--x"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_ok());
    CHECK(r.unwrap().get<bool, fixed_string("x")>() == true);
}

TEST_CASE("Parser subcommand with no args")
{
    App app("test", "1.0", "Sub no args");
    app.add_leaf("status", "Show status");
    Argv argv{"test", "status"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_ok());
    CHECK(r.unwrap().matched_path() == "status");
}

TEST_CASE("Parser parse_fuzzy with store extra args")
{
    App app("test", "1.0", "Fuzzy store");
    app.set_extra_args(ExtraArgsPolicy::Store);
    auto &srv = app.add_leaf("server", "Server");
    srv.arg<std::string, 0>("file", "File");
    Argv argv{"test", "servr", "data.txt", "extra1"};
    auto r = app.parse_fuzzy(argv.argc(), argv.argv());
    CHECK(r.is_ok());
    auto &ctx = r.unwrap();
    CHECK(ctx.matched_path() == "server");
    CHECK(ctx.get<std::string, 0>() == "data.txt");
    auto extra = ctx.extra_args();
    CHECK(extra.size() == 1);
    CHECK(extra[0] == "extra1");
}

// ──────────────────────────────────────────
//  Parent chain — subcommand reads parent values
// ──────────────────────────────────────────

TEST_CASE("Parent chain subcommand can read parent boolean flag")
{
    App app("test", "1.0", "Parent chain bool");
    app.option<fixed_string("verbose")>("--verbose", 'v', "Verbose").boolean();
    auto &son = app.add_leaf("son", "Son Command");
    son.action(
        [](ParseContext &ctx) -> CliResult<void>
        {
            CHECK(ctx.has<fixed_string("verbose")>());
            CHECK(ctx.get<bool, fixed_string("verbose")>() == true);
            return CliResult<void>::Ok();
        });
    LocalArgv argv{"test", "-v", "son"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_ok());
}

TEST_CASE("Parent chain subcommand can read parent int option")
{
    App app("test", "1.0", "Parent chain int");
    app.option<fixed_string("port")>("--port", 'p', "Port", 8080);
    auto &son = app.add_leaf("son", "Son Command");
    son.action(
        [](ParseContext &ctx) -> CliResult<void>
        {
            CHECK(ctx.has<fixed_string("port")>());
            CHECK(ctx.get<int, fixed_string("port")>() == 8080);
            return CliResult<void>::Ok();
        });
    LocalArgv argv{"test", "son"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_ok());
}

TEST_CASE("Parent chain subcommand can read parent string option from CLI")
{
    App app("test", "1.0", "Parent chain string");
    app.option<fixed_string("name")>("--name", "Name").str();
    auto &son = app.add_leaf("son", "Son Command");
    son.action(
        [](ParseContext &ctx) -> CliResult<void>
        {
            CHECK(ctx.has<fixed_string("name")>());
            CHECK(ctx.get<std::string, fixed_string("name")>() == "alice");
            return CliResult<void>::Ok();
        });
    LocalArgv argv{"test", "--name", "alice", "son"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_ok());
}

TEST_CASE("Parent chain subcommand can read parent bool with --no-xxx negation")
{
    App app("test", "1.0", "Parent chain negate");
    app.option<fixed_string("verbose")>("--verbose", 'v', "Verbose")
        .boolean()
        .negatable();
    auto &son = app.add_leaf("son", "Son Command");
    son.action(
        [](ParseContext &ctx) -> CliResult<void>
        {
            CHECK(ctx.has<fixed_string("verbose")>());
            CHECK(ctx.get<bool, fixed_string("verbose")>() == false);
            return CliResult<void>::Ok();
        });
    LocalArgv argv{"test", "--no-verbose", "son"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_ok());
}

TEST_CASE("Parent chain child has() returns false when parent option not given")
{
    App app("test", "1.0", "Parent chain absent");
    app.option<fixed_string("verbose")>("--verbose", 'v', "Verbose").boolean();
    auto &son = app.add_leaf("son", "Son Command");
    son.action(
        [](ParseContext &ctx) -> CliResult<void>
        {
            CHECK_FALSE(ctx.has<fixed_string("verbose")>());
            return CliResult<void>::Ok();
        });
    LocalArgv argv{"test", "son"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_ok());
}

TEST_CASE("Parent chain deep nesting reads root option from leaf")
{
    App app("test", "1.0", "Deep chain");
    app.option<fixed_string("verbose")>("--verbose", 'v', "Verbose").boolean();
    auto &mid = app.add_branch("mid", "Middle");
    auto &leaf = mid.add_leaf("leaf", "Leaf");
    leaf.action(
        [](ParseContext &ctx) -> CliResult<void>
        {
            CHECK(ctx.has<fixed_string("verbose")>());
            CHECK(ctx.get<bool, fixed_string("verbose")>() == true);
            return CliResult<void>::Ok();
        });
    LocalArgv argv{"test", "-v", "mid", "leaf"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_ok());
}

TEST_CASE("Parent chain child type fallback does not interfere with parent type")
{
    App app("test", "1.0", "Type isolation");
    app.option<fixed_string("port")>("--port", "Port", 8080);
    auto &son = app.add_leaf("son", "Son");
    son.option<fixed_string("host")>("--host", "Host", std::string("localhost"));
    son.action(
        [](ParseContext &ctx) -> CliResult<void>
        {
            CHECK(ctx.get<int, fixed_string("port")>() == 8080);
            CHECK(ctx.get<std::string, fixed_string("host")>() == "localhost");
            return CliResult<void>::Ok();
        });
    LocalArgv argv{"test", "son"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_ok());
}

TEST_CASE("Parent chain child get<T,Key>() after parse returns parent value")
{
    App app("test", "1.0", "Parent chain after parse");
    app.option<fixed_string("verbose")>("--verbose", 'v', "Verbose").boolean();
    auto &son = app.add_leaf("son", "Son");
    son.option<fixed_string("flag")>("--flag", 'f', "Flag").boolean();
    LocalArgv argv{"test", "-v", "son", "-f"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_ok());
    auto &ctx = r.unwrap();
    CHECK(ctx.has<fixed_string("verbose")>());
    CHECK(ctx.get<bool, fixed_string("verbose")>() == true);
    CHECK(ctx.get<bool, fixed_string("flag")>() == true);
}

// ──────────────────────────────────────────
//  Parent options before subcommand
// ──────────────────────────────────────────

TEST_CASE("Parser parent boolean flag consumed before subcommand")
{
    App app("test", "1.0", "Parent before sub");
    app.option<fixed_string("verbose")>("--verbose", 'v', "Verbose").boolean();
    auto &son = app.add_leaf("son", "Son");
    son.option<fixed_string("flag")>("--flag", 'f', "Flag").boolean();
    LocalArgv argv{"test", "-v", "son", "-f"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_ok());
    auto &ctx = r.unwrap();
    CHECK(ctx.matched_path() == "son");
    CHECK(ctx.get<bool, fixed_string("flag")>() == true);
}

TEST_CASE("Parser parent valued option consumed before subcommand")
{
    App app("test", "1.0", "Parent valued before sub");
    app.option<fixed_string("port")>("--port", "Port").integer();
    auto &son = app.add_leaf("son", "Son");
    son.option<fixed_string("flag")>("--flag", 'f', "Flag").boolean();
    LocalArgv argv{"test", "--port", "8080", "son", "-f"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_ok());
    auto &ctx = r.unwrap();
    CHECK(ctx.matched_path() == "son");
    CHECK(ctx.get<bool, fixed_string("flag")>() == true);
}

TEST_CASE("Parser parent options before leaf subcommand with arg")
{
    App app("test", "1.0", "Parent opt before leaf arg");
    app.option<fixed_string("verbose")>("--verbose", 'v', "Verbose").boolean();
    auto &son = app.add_leaf("son", "Son");
    son.arg<std::string, 0>("file", "File");
    son.action([](ParseContext &) -> CliResult<void> { return CliResult<void>::Ok(); });
    LocalArgv argv{"test", "-v", "son", "data.txt"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_ok());
    auto &ctx = r.unwrap();
    CHECK(ctx.matched_path() == "son");
    CHECK(ctx.get<bool, fixed_string("verbose")>() == true);
    CHECK(ctx.get<std::string, 0>() == "data.txt");
}

TEST_CASE("Parser double dash before subcommand stops lookup")
{
    App app("test", "1.0", "-- before sub");
    app.set_extra_args(ExtraArgsPolicy::Store);
    auto &son = app.add_branch("son", "Son");
    LocalArgv argv{"test", "--", "son"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_ok());
    // son should be stored as extra arg, not matched as subcommand
    auto &ctx = r.unwrap();
    auto extra = ctx.extra_args();
    CHECK(extra.size() == 1);
    CHECK(extra[0] == "son");
}
