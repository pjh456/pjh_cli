#include <doctest/doctest.h>

#include "test_helpers.hpp"

// ── Argv helper for standalone tests ──

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
    auto &serve = app.add_command("serve", "Start server");
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
    auto &db = app.add_command("db", "Database commands");
    auto &migrate = db.add_command("migrate", "Run migrations");
    migrate.option<fixed_string("name")>("--name", 'n', "Migration name").str();
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
    active.option<fixed_string("x")>("--x", 'x', "Flag").boolean();
    app.add_command("disabled", "Unavailable").enabled([] { return false; });
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

// ──────────────────────────────────────────
//  Parent args before subcommand
// ──────────────────────────────────────────

TEST_CASE("Parser parent boolean flag consumed before subcommand")
{
    App app("test", "1.0", "Parent before sub");
    app.option<fixed_string("verbose")>("--verbose", 'v', "Verbose").boolean();
    auto &son = app.add_command("son", "Son");
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
    auto &son = app.add_command("son", "Son");
    son.option<fixed_string("flag")>("--flag", 'f', "Flag").boolean();
    LocalArgv argv{"test", "--port", "8080", "son", "-f"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_ok());
    auto &ctx = r.unwrap();
    CHECK(ctx.matched_path() == "son");
    CHECK(ctx.get<bool, fixed_string("flag")>() == true);
}

TEST_CASE("Parser parent positional consumed before subcommand")
{
    App app("test", "1.0", "Parent positional before sub");
    app.arg<std::string, 0>("file", "File");
    auto &son = app.add_command("son", "Son");
    son.action([](ParseContext &) -> CliResult<void> { return CliResult<void>::Ok(); });
    LocalArgv argv{"test", "data.txt", "son"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_ok());
    CHECK(r.unwrap().matched_path() == "son");
}

TEST_CASE("Parser double dash before subcommand stops lookup")
{
    App app("test", "1.0", "-- before sub");
    app.set_extra_args(ExtraArgsPolicy::Store);
    app.arg<std::string, 0>("file", "File");
    auto &son = app.add_command("son", "Son");
    LocalArgv argv{"test", "--", "son"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_ok());
    CHECK(r.unwrap().get<std::string, 0>() == "son");
}
