#include <doctest/doctest.h>
#include "test_helpers.hpp"

TEST_CASE("Parser subcommand matching")
{
    App app("test", "1.0", "Subcommand test");
    auto &serve = app.add_command("serve", "Start server");
    serve.option<int, fixed_string("port")>("--port", 'p', "Port");
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
    auto &db = app.add_command("db", "Database commands");
    auto &migrate = db.add_command("migrate", "Run migrations");
    migrate.option<std::string, fixed_string("name")>("--name", 'n', "Migration name");
    Argv argv{"test", "db", "migrate", "--name", "v2"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_ok());
    auto &ctx = r.unwrap();
    CHECK(ctx.get<std::string, fixed_string("name")>() == "v2");
    CHECK(ctx.matched_path() == "migrate");
}

TEST_CASE("Parser disabled subcommand skipped")
{
    App app("test", "1.0", "Disabled test");
    auto &active = app.add_command("active", "Available");
    active.option<bool, fixed_string("x")>("--x", 'x', "Flag");
    app.add_command("disabled", "Unavailable")
        .enabled([] { return false; });
    Argv argv{"test", "active", "--x"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_ok());
    CHECK(r.unwrap().get<bool, fixed_string("x")>() == true);
}

TEST_CASE("Parser subcommand with no args")
{
    App app("test", "1.0", "Sub no args");
    app.add_command("status", "Show status");
    Argv argv{"test", "status"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_ok());
    CHECK(r.unwrap().matched_path() == "status");
}

TEST_CASE("Parser parse_fuzzy with store extra args")
{
    App app("test", "1.0", "Fuzzy store");
    app.set_extra_args(ExtraArgsPolicy::Store);
    auto &srv = app.add_command("server", "Server");
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
