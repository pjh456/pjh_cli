#include <doctest/doctest.h>

#include <initializer_list>
#include <iostream>
#include <pjh_cli/app.hpp>
#include <pjh_cli/command/base_command.hpp>
#include <pjh_cli/core/error.hpp>
#include <pjh_cli/core/fixed_string.hpp>
#include <pjh_cli/core/type.hpp>
#include <pjh_cli/parse/matched_path_resolver.hpp>
#include <string>
#include <string_view>
#include <variant>
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
    CHECK(MatchedPathResolver::to_path_string(ctx.matched_command()) == "serve");
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
    CHECK(MatchedPathResolver::to_path_string(ctx.matched_command()) == "db migrate");
}

TEST_CASE("MatchedPathResolver::to_path_info returns commands")
{
    App app("test", "1.0", "Path info test");
    auto &db = app.add_branch("db", "Database commands");
    db.add_leaf("migrate", "Run migrations");
    Argv argv{"test", "db", "migrate"};
    auto r = app.parse(argv.argc(), argv.argv());
    REQUIRE(r.is_ok());
    MatchedPath path = MatchedPathResolver::to_path_info(r.unwrap().matched_command());
    REQUIRE(path.commands.size() == 2);
    CHECK(path.commands[0] == "db");
    CHECK(path.commands[1] == "migrate");
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
    CHECK(MatchedPathResolver::to_path_string(r.unwrap().matched_command()) == "status");
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
    CHECK(MatchedPathResolver::to_path_string(ctx.matched_command()) == "server");
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

TEST_CASE("Parent chain negated flag with value rejects before descent")
{
    App app("test", "1.0", "Parent chain negate eq");
    app.option<fixed_string("verbose")>("--verbose", 'v', "Verbose")
        .boolean()
        .negatable();
    app.add_leaf("son", "Son Command");
    LocalArgv argv{"test", "--no-verbose=0", "son"};
    auto r = app.parse(argv.argc(), argv.argv());
    REQUIRE(r.is_err());
    CHECK(
        r.unwrap_err().what() ==
        std::string_view("Parse Error: option '--no-verbose' does not accept a value"));
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
    CHECK(MatchedPathResolver::to_path_string(ctx.matched_command()) == "son");
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
    CHECK(MatchedPathResolver::to_path_string(ctx.matched_command()) == "son");
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
    CHECK(MatchedPathResolver::to_path_string(ctx.matched_command()) == "son");
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

// ──────────────────────────────────────────
//  Subcommand aliases
// ──────────────────────────────────────────

TEST_CASE("Parser alias exact match")
{
    App app("test", "1.0", "Alias test");
    auto &serve = app.add_leaf("serve", "Start server");
    serve.alias("run");
    serve.option<fixed_string("port")>("--port", 'p', "Port").integer();
    Argv argv{"test", "run", "--port", "8080"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_ok());
    CHECK(r.unwrap().get<int, fixed_string("port")>() == 8080);
    CHECK(MatchedPathResolver::to_path_string(r.unwrap().matched_command()) == "serve");
}

TEST_CASE("Parser alias both names work")
{
    App app("test", "1.0", "Alias both");
    auto &cmd = app.add_leaf("list", "List items");
    cmd.alias("ls");
    cmd.alias("show");
    cmd.option<fixed_string("all")>("--all", 'a', "Show all").boolean();

    Argv argv1{"test", "list", "--all"};
    auto r1 = app.parse(argv1.argc(), argv1.argv());
    CHECK(r1.is_ok());
    CHECK(MatchedPathResolver::to_path_string(r1.unwrap().matched_command()) == "list");

    Argv argv2{"test", "ls", "--all"};
    auto r2 = app.parse(argv2.argc(), argv2.argv());
    CHECK(r2.is_ok());
    CHECK(MatchedPathResolver::to_path_string(r2.unwrap().matched_command()) == "list");

    Argv argv3{"test", "show"};
    auto r3 = app.parse(argv3.argc(), argv3.argv());
    CHECK(r3.is_ok());
    CHECK(MatchedPathResolver::to_path_string(r3.unwrap().matched_command()) == "list");
}

TEST_CASE("Parser alias on branch subcommand")
{
    App app("test", "1.0", "Alias branch");
    auto &db = app.add_branch("database", "Database commands");
    db.alias("db");
    auto &migrate = db.add_leaf("migrate", "Run migrations");

    Argv argv{"test", "db", "migrate"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_ok());
    CHECK(
        MatchedPathResolver::to_path_string(r.unwrap().matched_command()) ==
        "database migrate");
}

TEST_CASE("Parser alias with fuzzy match")
{
    App app("test", "1.0", "Alias fuzzy");
    auto &serve = app.add_leaf("serve", "Start server");
    serve.alias("start");

    Argv argv{"test", "stert"};
    auto r = app.parse_fuzzy(argv.argc(), argv.argv());
    CHECK(r.is_ok());
    CHECK(MatchedPathResolver::to_path_string(r.unwrap().matched_command()) == "serve");
}

// ──────────────────────────────────────────
//  Repeatable greedy stops at subcommand boundary
// ──────────────────────────────────────────

TEST_CASE("Repeatable greedy stops at subcommand name")
{
    App app("test", "1.0", "Greedy sub long");
    app.option<fixed_string("human")>("--human", "Human seat").str().repeatable();
    app.add_leaf("new", "New");
    Argv argv{"test", "--human", "P0", "new"};
    auto r = app.parse(argv.argc(), argv.argv());
    REQUIRE(r.is_ok());
    auto &ctx = r.unwrap();
    CHECK(MatchedPathResolver::to_path_string(ctx.matched_command()) == "new");
    auto all = ctx.get_all<std::string, fixed_string("human")>();
    REQUIRE(all.size() == 1);
    CHECK(all[0] == "P0");
}

TEST_CASE("Repeatable greedy short option stops at subcommand name")
{
    App app("test", "1.0", "Greedy sub short");
    app.option<fixed_string("human")>("--human", 'H', "Human seat").str().repeatable();
    app.add_leaf("new", "New");
    Argv argv{"test", "-H", "P0", "new"};
    auto r = app.parse(argv.argc(), argv.argv());
    REQUIRE(r.is_ok());
    auto &ctx = r.unwrap();
    CHECK(MatchedPathResolver::to_path_string(ctx.matched_command()) == "new");
    auto all = ctx.get_all<std::string, fixed_string("human")>();
    REQUIRE(all.size() == 1);
    CHECK(all[0] == "P0");
}

TEST_CASE("Repeatable greedy deep nesting stops at every subcommand boundary")
{
    App app("test", "1.0", "Greedy sub deep");
    app.option<fixed_string("human")>("--human", "Human seat").str().repeatable();
    auto &mid = app.add_branch("mid", "Middle");
    mid.option<fixed_string("leg")>("--leg", 'l', "Leg seat").str().repeatable();
    mid.option<fixed_string("arm")>("--arm", 'a', "Arm seat").str().repeatable();
    mid.add_leaf("leaf", "Leaf");
    Argv argv{"test", "--human", "P0", "mid", "-l", "L0", "-aL1", "leaf"};
    auto r = app.parse(argv.argc(), argv.argv());
    REQUIRE(r.is_ok());
    auto &ctx = r.unwrap();
    CHECK(MatchedPathResolver::to_path_string(ctx.matched_command()) == "mid leaf");
    auto human = ctx.get_all<std::string, fixed_string("human")>();
    REQUIRE(human.size() == 1);
    CHECK(human[0] == "P0");
    auto leg = ctx.get_all<std::string, fixed_string("leg")>();
    REQUIRE(leg.size() == 1);
    CHECK(leg[0] == "L0");
    auto arm = ctx.get_all<std::string, fixed_string("arm")>();
    REQUIRE(arm.size() == 1);
    CHECK(arm[0] == "L1");
}

// ──────────────────────────────────────────
//  Ancestor options after subcommand descent
// ──────────────────────────────────────────

TEST_CASE("Ancestor long option after subcommand descent")
{
    App app("test", "1.0", "Ancestor long after");
    app.option<fixed_string("port")>("--port", 'p', "Port").integer();
    app.add_leaf("son", "Son");
    Argv argv{"test", "son", "--port", "8080"};
    auto r = app.parse(argv.argc(), argv.argv());
    REQUIRE(r.is_ok());
    auto &ctx = r.unwrap();
    CHECK(MatchedPathResolver::to_path_string(ctx.matched_command()) == "son");
    CHECK(ctx.get<int, fixed_string("port")>() == 8080);
}

TEST_CASE("Ancestor short option after subcommand descent")
{
    App app("test", "1.0", "Ancestor short after");
    app.option<fixed_string("port")>("--port", 'p', "Port").integer();
    app.add_leaf("son", "Son");
    Argv argv{"test", "son", "-p", "8080"};
    auto r = app.parse(argv.argc(), argv.argv());
    REQUIRE(r.is_ok());
    CHECK(r.unwrap().get<int, fixed_string("port")>() == 8080);
}

TEST_CASE("Ancestor boolean flag after subcommand descent")
{
    App app("test", "1.0", "Ancestor flag after");
    app.option<fixed_string("verbose")>("--verbose", 'v', "Verbose").boolean();
    app.add_leaf("son", "Son");
    Argv argv{"test", "son", "--verbose"};
    auto r = app.parse(argv.argc(), argv.argv());
    REQUIRE(r.is_ok());
    CHECK(r.unwrap().get<bool, fixed_string("verbose")>() == true);
}

TEST_CASE("Ancestor negatable flag after subcommand descent")
{
    App app("test", "1.0", "Ancestor negate after");
    app.option<fixed_string("verbose")>("--verbose", 'v', "Verbose")
        .boolean()
        .negatable();
    app.add_leaf("son", "Son");
    Argv argv{"test", "son", "--no-verbose"};
    auto r = app.parse(argv.argc(), argv.argv());
    REQUIRE(r.is_ok());
    CHECK(r.unwrap().get<bool, fixed_string("verbose")>() == false);
}

TEST_CASE("Root option after deep nesting descent")
{
    App app("test", "1.0", "Deep ancestor after");
    app.option<fixed_string("port")>("--port", 'p', "Port").integer();
    auto &mid = app.add_branch("mid", "Middle");
    mid.add_leaf("leaf", "Leaf");
    Argv argv{"test", "mid", "leaf", "--port", "9"};
    auto r = app.parse(argv.argc(), argv.argv());
    REQUIRE(r.is_ok());
    CHECK(
        MatchedPathResolver::to_path_string(r.unwrap().matched_command()) == "mid leaf");
    CHECK(r.unwrap().get<int, fixed_string("port")>() == 9);
}

TEST_CASE("Intermediate branch option after deeper descent")
{
    App app("test", "1.0", "Mid ancestor after");
    auto &mid = app.add_branch("mid", "Middle");
    mid.option<fixed_string("leg")>("--leg", 'l', "Leg").str();
    mid.add_leaf("leaf", "Leaf");
    Argv argv{"test", "mid", "leaf", "--leg", "L0"};
    auto r = app.parse(argv.argc(), argv.argv());
    REQUIRE(r.is_ok());
    CHECK(r.unwrap().get<std::string, fixed_string("leg")>() == "L0");
}

TEST_CASE("Repeatable ancestor option accumulates across descent")
{
    App app("test", "1.0", "Ancestor repeat after");
    app.option<fixed_string("tag")>("--tag", 't', "Tag").str().repeatable();
    app.add_leaf("son", "Son");
    Argv argv{"test", "--tag", "a", "son", "--tag", "b"};
    auto r = app.parse(argv.argc(), argv.argv());
    REQUIRE(r.is_ok());
    auto all = r.unwrap().get_all<std::string, fixed_string("tag")>();
    REQUIRE(all.size() == 2);
    CHECK(all[0] == "a");
    CHECK(all[1] == "b");
}

TEST_CASE("Counting ancestor option accumulates across descent")
{
    App app("test", "1.0", "Ancestor count after");
    app.option<fixed_string("v")>("--verbose", 'v', "Verbosity").count();
    app.add_leaf("son", "Son");
    Argv argv{"test", "-v", "son", "-v"};
    auto r = app.parse(argv.argc(), argv.argv());
    REQUIRE(r.is_ok());
    CHECK(r.unwrap().get<int, fixed_string("v")>() == 2);
}

TEST_CASE("Nearest declaration wins when names collide")
{
    App app("test", "1.0", "Nearest wins");
    app.option<fixed_string("root_opt")>("--opt", "Root opt").integer();
    auto &son = app.add_leaf("son", "Son");
    son.option<fixed_string("child_opt")>("--opt", "Child opt").integer();
    Argv argv{"test", "son", "--opt", "7"};
    auto r = app.parse(argv.argc(), argv.argv());
    REQUIRE(r.is_ok());
    auto &ctx = r.unwrap();
    CHECK(ctx.get<int, fixed_string("child_opt")>() == 7);
    CHECK_FALSE(ctx.has<fixed_string("root_opt")>());
}

TEST_CASE("Sibling option is not visible after descent")
{
    App app("test", "1.0", "Sibling invisible");
    app.add_leaf("a", "A");
    auto &b = app.add_leaf("b", "B");
    b.option<fixed_string("bopt")>("--bopt", "B opt").boolean();
    Argv argv{"test", "a", "--bopt"};
    auto r = app.parse(argv.argc(), argv.argv());
    REQUIRE(r.is_err());
    CHECK(
        r.unwrap_err().what() ==
        std::string_view("Parse Error: unknown option: '--bopt'"));
}

TEST_CASE("Unknown option after descent still errors")
{
    App app("test", "1.0", "Unknown after");
    app.add_leaf("son", "Son");
    Argv argv{"test", "son", "--bogus"};
    auto r = app.parse(argv.argc(), argv.argv());
    REQUIRE(r.is_err());
    CHECK(
        r.unwrap_err().what() ==
        std::string_view("Parse Error: unknown option: '--bogus'"));
}

TEST_CASE("Parser unknown long option suggests ancestor option")
{
    App app("test", "1.0", "Ancestor suggestion");
    app.option<fixed_string("verbose")>("--verbose", "Verbose").boolean();
    app.add_leaf("son", "Son");
    Argv argv{"test", "son", "--verbos"};
    auto r = app.parse(argv.argc(), argv.argv());
    REQUIRE(r.is_err());
    auto msg = std::string_view(r.unwrap_err().what());
    CHECK(msg.find("unknown option: '--verbos'") != std::string_view::npos);
    CHECK(msg.find("did you mean: --verbose") != std::string_view::npos);
}

// ──────────────────────────────────────────
//  Ancestor options: finalization interaction
// ──────────────────────────────────────────

TEST_CASE("Required ancestor option satisfied after descent")
{
    App app("test", "1.0", "Required ancestor after");
    app.option<fixed_string("token")>("--token", "Token").str().required();
    app.add_leaf("son", "Son");
    Argv argv{"test", "son", "--token", "t0"};
    auto r = app.parse(argv.argc(), argv.argv());
    REQUIRE(r.is_ok());
    CHECK(r.unwrap().get<std::string, fixed_string("token")>() == "t0");
}

TEST_CASE("Required ancestor option still enforced when absent after descent")
{
    App app("test", "1.0", "Required ancestor absent");
    app.option<fixed_string("token")>("--token", "Token").str().required();
    app.add_leaf("son", "Son");
    Argv argv{"test", "son"};
    auto r = app.parse(argv.argc(), argv.argv());
    REQUIRE(r.is_err());
    CHECK(
        r.unwrap_err().what() ==
        std::string_view("Parse Error: missing required option: '--token'"));
}

TEST_CASE("Ancestor option conversion error after descent")
{
    App app("test", "1.0", "Ancestor convert after");
    app.option<fixed_string("port")>("--port", "Port").integer();
    app.add_leaf("son", "Son");
    Argv argv{"test", "son", "--port", "notanint"};
    auto r = app.parse(argv.argc(), argv.argv());
    REQUIRE(r.is_err());
    CHECK(
        std::string_view(r.unwrap_err().what()).find("--port") != std::string_view::npos);
}

// ──────────────────────────────────────────
//  Extra args accumulate across subcommand descent (task 37)
// ──────────────────────────────────────────

TEST_CASE("Store extra args before subcommand descent are preserved")
{
    App app("test", "1.0", "Store across descent");
    app.set_extra_args(ExtraArgsPolicy::Store);
    app.add_leaf("sub", "Sub");
    Argv argv{"test", "foo", "sub", "bar"};
    auto r = app.parse(argv.argc(), argv.argv());
    REQUIRE(r.is_ok());
    auto extra = r.unwrap().extra_args();
    REQUIRE(extra.size() == 2);
    CHECK(extra[0] == "foo");
    CHECK(extra[1] == "bar");
}

TEST_CASE("Store extra args accumulate across deep nesting")
{
    App app("test", "1.0", "Store deep");
    app.set_extra_args(ExtraArgsPolicy::Store);
    auto &mid = app.add_branch("mid", "Middle");
    mid.add_leaf("leaf", "Leaf");
    Argv argv{"test", "a", "mid", "b", "leaf", "c"};
    auto r = app.parse(argv.argc(), argv.argv());
    REQUIRE(r.is_ok());
    auto extra = r.unwrap().extra_args();
    REQUIRE(extra.size() == 3);
    CHECK(extra[0] == "a");
    CHECK(extra[1] == "b");
    CHECK(extra[2] == "c");
}

TEST_CASE("Store extra args coexist with leaf positional after descent")
{
    App app("test", "1.0", "Store positional");
    app.set_extra_args(ExtraArgsPolicy::Store);
    auto &sub = app.add_leaf("sub", "Sub");
    sub.arg<std::string, 0>("file", "File");
    Argv argv{"test", "foo", "sub", "data.txt", "bar"};
    auto r = app.parse(argv.argc(), argv.argv());
    REQUIRE(r.is_ok());
    auto &ctx = r.unwrap();
    CHECK(ctx.get<std::string, 0>() == "data.txt");
    auto extra = ctx.extra_args();
    REQUIRE(extra.size() == 2);
    CHECK(extra[0] == "foo");
    CHECK(extra[1] == "bar");
}

TEST_CASE("Store extra arg after descent with leaf-only policy")
{
    App app("test", "1.0", "Store leaf only");
    auto &sub = app.add_leaf("sub", "Sub");
    sub.set_extra_args(ExtraArgsPolicy::Store);
    Argv argv{"test", "sub", "bar"};
    auto r = app.parse(argv.argc(), argv.argv());
    REQUIRE(r.is_ok());
    auto extra = r.unwrap().extra_args();
    REQUIRE(extra.size() == 1);
    CHECK(extra[0] == "bar");
}

TEST_CASE("Store extra args with double dash before descent")
{
    App app("test", "1.0", "Store double dash");
    app.set_extra_args(ExtraArgsPolicy::Store);
    app.add_leaf("sub", "Sub");
    Argv argv{"test", "--", "foo", "sub"};
    auto r = app.parse(argv.argc(), argv.argv());
    REQUIRE(r.is_ok());
    auto extra = r.unwrap().extra_args();
    REQUIRE(extra.size() == 2);
    CHECK(extra[0] == "foo");
    CHECK(extra[1] == "sub");
}

TEST_CASE("Explicit ignore before subcommand descent discards token")
{
    App app("test", "1.0", "Ignore before descent");
    app.set_extra_args(ExtraArgsPolicy::Ignore);
    app.add_leaf("sub", "Sub");
    Argv argv{"test", "foo", "sub", "bar"};
    auto r = app.parse(argv.argc(), argv.argv());
    REQUIRE(r.is_ok());
    auto &ctx = r.unwrap();
    CHECK(MatchedPathResolver::to_path_string(ctx.matched_command()) == "sub");
    CHECK(ctx.extra_args().empty());
}

TEST_CASE("Error policy before subcommand descent still errors")
{
    App app("test", "1.0", "Error before descent");
    app.set_extra_args(ExtraArgsPolicy::Error);
    app.add_leaf("sub", "Sub");
    Argv argv{"test", "foo", "sub"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_err());
}

TEST_CASE("Store extra args across fuzzy descent")
{
    App app("test", "1.0", "Store fuzzy descent");
    app.set_extra_args(ExtraArgsPolicy::Store);
    app.add_leaf("server", "Server");
    Argv argv{"test", "foo", "servr", "bar"};
    auto r = app.parse_fuzzy(argv.argc(), argv.argv());
    REQUIRE(r.is_ok());
    auto extra = r.unwrap().extra_args();
    REQUIRE(extra.size() == 2);
    CHECK(extra[0] == "foo");
    CHECK(extra[1] == "bar");
}

TEST_CASE("Dash token still receives unknown command suggestions")
{
    App app("test", "1.0", "Dash suggestion");
    app.add_leaf("5", "Five");
    Argv argv{"test", "-5"};
    auto r = app.parse_fuzzy(argv.argc(), argv.argv());
    REQUIRE(r.is_err());
    CHECK(std::holds_alternative<UnknownCommandError>(r.unwrap_err().info()));
    auto msg = std::string_view(r.unwrap_err().what());
    CHECK(msg.find("unknown command: '-5'") != std::string_view::npos);
    // The dash token takes the resolver's early return, so no fuzzy pass ran
    // and the suggestions_known_empty flag stays false: the distance-3
    // suggestion must still be computed.
    CHECK(msg.find("did you mean: 5") != std::string_view::npos);
}

TEST_CASE("Long unknown command receives no suggestions")
{
    App app("test", "1.0", "Long token");
    app.add_leaf("server", "Server");
    Argv argv{"test", "xxxxxxxxxxxxxxxx"};
    auto r = app.parse_fuzzy(argv.argc(), argv.argv());
    REQUIRE(r.is_err());
    CHECK(std::holds_alternative<UnknownCommandError>(r.unwrap_err().info()));
    auto msg = std::string_view(r.unwrap_err().what());
    CHECK(msg.find("unknown command: 'xxxxxxxxxxxxxxxx'") != std::string_view::npos);
    CHECK(msg.find("did you mean") == std::string_view::npos);
}
