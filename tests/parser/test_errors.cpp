#include <doctest/doctest.h>

#include <algorithm>
#include <iostream>
#include <pjh_cli/app.hpp>
#include <pjh_cli/command/leaf_command.hpp>
#include <pjh_cli/core/error.hpp>
#include <pjh_cli/core/fixed_string.hpp>
#include <pjh_cli/parse/parse_finalizer.hpp>
#include <pjh_cli/parse/parser.hpp>
#include <string_view>
#include <utility>
#include <variant>

#include "test_helpers.hpp"

TEST_CASE("Parser unknown long option")
{
    App app("test", "1.0", "Unknown test");
    Argv argv{"test", "--bogus"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_err());
}

TEST_CASE("Parser unknown short option")
{
    App app("test", "1.0", "Unknown short test");
    Argv argv{"test", "-x"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_err());
}

TEST_CASE("Parser missing value for long option")
{
    App app("test", "1.0", "Missing value test");
    app.option<fixed_string("port")>("--port", "Port").integer();
    Argv argv{"test", "--port"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_err());
}

TEST_CASE("Parser missing value for short option")
{
    App app("test", "1.0", "Missing short value test");
    app.option<fixed_string("port")>("--port", 'p', "Port").integer();
    Argv argv{"test", "-p"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_err());
}

TEST_CASE("Parser required option missing")
{
    App app("test", "1.0", "Required opt test");
    app.option<fixed_string("port")>("--port", "Port").integer().required();
    Argv argv{"test"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_err());
}

TEST_CASE("Parser required positional arg missing")
{
    LeafCommand root("test", "Required arg test");
    root.arg<std::string, 0>("file", "Input file").required();
    Argv argv{"test"};
    auto r = Parser::parse_command(root, argv.argc(), argv.argv());
    CHECK(r.is_err());
}

TEST_CASE("Parser type conversion failure")
{
    App app("test", "1.0", "Conversion test");
    app.option<fixed_string("port")>("--port", "Port").integer();
    Argv argv{"test", "--port", "notanumber"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_err());
}

TEST_CASE("Parser required option and required arg both missing")
{
    LeafCommand root("test", "Both required missing");
    root.option<fixed_string("port")>("--port", "Port").integer().required();
    root.arg<std::string, 0>("file", "Input file").required();
    Argv argv{"test"};
    auto r = Parser::parse_command(root, argv.argc(), argv.argv());
    CHECK(r.is_err());
}

TEST_CASE("Parser error message unknown long option")
{
    App app("test", "1.0", "Err msg");
    Argv argv{"test", "--bogus"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_err());
    CHECK(
        r.unwrap_err().what() ==
        std::string_view("Parse Error: unknown option: '--bogus'"));
}

TEST_CASE("Parser error message unknown short option")
{
    App app("test", "1.0", "Err msg");
    Argv argv{"test", "-x"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_err());
    CHECK(r.unwrap_err().what() == std::string_view("Parse Error: unknown option: '-x'"));
}

TEST_CASE("Parser unknown long option suggests close option")
{
    App app("test", "1.0", "Suggestion");
    app.option<fixed_string("port")>("--port", "Port").integer();
    Argv argv{"test", "--prot"};
    auto r = app.parse(argv.argc(), argv.argv());
    REQUIRE(r.is_err());
    CHECK(
        r.unwrap_err().what() ==
        std::string_view("Parse Error: unknown option: '--prot'; did you mean: --port"));
}

TEST_CASE("Parser unknown long option without close option keeps message")
{
    App app("test", "1.0", "No suggestion");
    app.option<fixed_string("port")>("--port", "Port").integer();
    Argv argv{"test", "--bogus"};
    auto r = app.parse(argv.argc(), argv.argv());
    REQUIRE(r.is_err());
    CHECK(
        r.unwrap_err().what() ==
        std::string_view("Parse Error: unknown option: '--bogus'"));
}

TEST_CASE("Parser unknown long option suggestions are capped")
{
    App app("test", "1.0", "Cap");
    app.option<fixed_string("port")>("--port", "Port").integer();
    app.option<fixed_string("pork")>("--pork", "Pork").integer();
    app.option<fixed_string("poor")>("--poor", "Poor").integer();
    app.option<fixed_string("pot")>("--pot", "Pot").integer();
    Argv argv{"test", "--por"};
    auto r = app.parse(argv.argc(), argv.argv());
    REQUIRE(r.is_err());
    auto msg = std::string_view(r.unwrap_err().what());
    CHECK(msg.find("did you mean:") != std::string_view::npos);
    // All four are distance 1 from "por"; stable registration order keeps the
    // first three and drops the fourth.
    CHECK(msg.find("--port") != std::string_view::npos);
    CHECK(msg.find("--pork") != std::string_view::npos);
    CHECK(msg.find("--poor") != std::string_view::npos);
    CHECK(msg.find("--pot") == std::string_view::npos);  // 4th dropped
}

TEST_CASE("Parser unknown long option with very long token has no suggestions")
{
    App app("test", "1.0", "Long token");
    app.option<fixed_string("port")>("--port", "Port").integer();
    Argv argv{"test", "--xxxxxxxxxxxxxxxx"};
    auto r = app.parse(argv.argc(), argv.argv());
    REQUIRE(r.is_err());
    CHECK(
        r.unwrap_err().what() ==
        std::string_view("Parse Error: unknown option: '--xxxxxxxxxxxxxxxx'"));
}

TEST_CASE("Parser unknown long option skips hidden command options")
{
    App app("test", "1.0", "Hidden");
    auto &secret = app.add_leaf("secret", "Secret");
    secret.set_visibility(Visibility::Hidden);
    secret.option<fixed_string("token")>("--token", "Token").str();
    Argv argv{"test", "secret", "--toke"};
    auto r = app.parse(argv.argc(), argv.argv());
    REQUIRE(r.is_err());
    CHECK(
        r.unwrap_err().what() ==
        std::string_view("Parse Error: unknown option: '--toke'"));
}

TEST_CASE("Parser error message missing value for long option")
{
    App app("test", "1.0", "Err msg");
    app.option<fixed_string("port")>("--port", "Port").integer();
    Argv argv{"test", "--port"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_err());
    CHECK(
        r.unwrap_err().what() ==
        std::string_view("Parse Error: option '--port' requires a value"));
}

TEST_CASE("Parser error message missing value for short option")
{
    App app("test", "1.0", "Err msg");
    app.option<fixed_string("port")>("--port", 'p', "Port").integer();
    Argv argv{"test", "-p"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_err());
    CHECK(
        r.unwrap_err().what() ==
        std::string_view("Parse Error: option '-p' requires a value"));
}

TEST_CASE("Parser error message required option missing")
{
    App app("test", "1.0", "Err msg");
    app.option<fixed_string("port")>("--port", "Port").integer().required();
    Argv argv{"test"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_err());
    CHECK(
        r.unwrap_err().what() ==
        std::string_view("Parse Error: missing required option: '--port'"));
}

TEST_CASE("Parser error message required arg missing")
{
    LeafCommand root("test", "Err msg");
    root.arg<std::string, 0>("file", "Input file").required();
    Argv argv{"test"};
    auto r = Parser::parse_command(root, argv.argc(), argv.argv());
    CHECK(r.is_err());
    CHECK(
        r.unwrap_err().what() ==
        std::string_view("Parse Error: missing required argument: 'file'"));
}

TEST_CASE("Parser error message type conversion failure")
{
    App app("test", "1.0", "Err msg");
    app.option<fixed_string("port")>("--port", "Port").integer();
    Argv argv{"test", "--port", "notanumber"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_err());
    CHECK(
        r.unwrap_err().what() ==
        std::string_view(
            "Parse Error: invalid value 'notanumber' for '--port': expected integer"));
}

namespace
{
    enum class ErrorColor
    {
        red,
        green
    };
}

TEST_CASE("Parser error message enum invalid value")
{
    App app("test", "1.0", "Err msg");
    app.option<fixed_string("color")>("--color", 'c', "Color")
        .enum_type<ErrorColor>()
        .mapping({{"red", ErrorColor::red}, {"green", ErrorColor::green}});
    Argv argv{"test", "--color", "yellow"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_err());
    auto msg = std::string_view(r.unwrap_err().what());
    CHECK(
        msg.find("for '--color': expected one of: red, green") != std::string_view::npos);
}

TEST_CASE("Parser error message disabled command")
{
    App app("test", "1.0", "Err msg");
    app.add_branch("oldcmd", "Deprecated").enabled([] { return false; });
    Argv argv{"test", "oldcmd"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_err());
    CHECK(
        r.unwrap_err().what() ==
        std::string_view("Parse Error: command 'oldcmd' is not available"));
}

TEST_CASE("Parser unmatched subcommand errors")
{
    App app("test", "1.0", "Unmatched subcommand");
    app.add_leaf("install", "Install");
    Argv argv{"test", "instal"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_err());
}

TEST_CASE("Parser unknown command message")
{
    App app("test", "1.0", "Unknown command message");
    app.add_leaf("install", "Install");
    Argv argv{"test", "zzzz"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_err());
    CHECK(
        r.unwrap_err().what() ==
        std::string_view("Parse Error: unknown command: 'zzzz'"));
}

TEST_CASE("Parser unmatched subcommand suggests close name")
{
    App app("test", "1.0", "Unknown command suggestion");
    app.add_leaf("install", "Install");
    Argv argv{"test", "instal"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_err());
    auto msg = std::string_view(r.unwrap_err().what());
    CHECK(msg.find("Parse Error: unknown command: 'instal'") != std::string_view::npos);
    CHECK(msg.find("install") != std::string_view::npos);
}

TEST_CASE("Parser unmatched subcommand explicit ignore succeeds")
{
    App app("test", "1.0", "Explicit ignore");
    app.set_extra_args(ExtraArgsPolicy::Ignore);
    app.add_leaf("install", "Install");
    Argv argv{"test", "instal"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_ok());
}

TEST_CASE("Parser unmatched subcommand store policy stores")
{
    App app("test", "1.0", "Explicit store");
    app.set_extra_args(ExtraArgsPolicy::Store);
    app.add_leaf("install", "Install");
    Argv argv{"test", "instal"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_ok());
    auto extra = r.unwrap().extra_args();
    REQUIRE(extra.size() == 1);
    CHECK(extra[0] == "instal");
}

TEST_CASE("Parser branch without subcommands ignores extra token")
{
    App app("test", "1.0", "Root action");
    int called = 0;
    app.action(
        [&called](ParseContext &) -> CliResult<void>
        {
            ++called;
            return CliResult<void>::Ok();
        });
    Argv argv{"test", "somearg"};
    auto r = app.parse(argv.argc(), argv.argv());
    REQUIRE(r.is_ok());
    auto &ctx = r.unwrap();
    CHECK(ctx.matched_command()->execute(ctx).is_ok());
    CHECK(called == 1);
}

TEST_CASE("Parser unmatched nested subcommand errors")
{
    App app("test", "1.0", "Nested unmatched");
    auto &db = app.add_branch("db", "Database commands");
    db.add_leaf("migrate", "Run migrations");
    Argv argv{"test", "db", "migrat"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_err());
}

TEST_CASE("Parser unmatched subcommand after double dash honors store")
{
    App app("test", "1.0", "Double dash store");
    app.set_extra_args(ExtraArgsPolicy::Store);
    app.add_leaf("install", "Install");
    Argv argv{"test", "--", "instal"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_ok());
    auto extra = r.unwrap().extra_args();
    REQUIRE(extra.size() == 1);
    CHECK(extra[0] == "instal");
}

TEST_CASE("ParseFinalizer rejects null command")
{
    ParseContext ctx;
    CHECK_THROWS_AS((void)ParseFinalizer::finalize(nullptr, std::move(ctx)), LogicError);
}

TEST_CASE("Parser fuzzy typo of a disabled command reports disabled")
{
    App app("test", "1.0", "Err msg");
    app.add_branch("oldcmd", "Deprecated").enabled([] { return false; });
    Argv argv{"test", "oldcm"};
    auto r = app.parse_fuzzy(argv.argc(), argv.argv());
    CHECK(r.is_err());
    CHECK(std::holds_alternative<CommandDisabledError>(r.unwrap_err().info()));
    CHECK(
        r.unwrap_err().what() ==
        std::string_view("Parse Error: command 'oldcmd' is not available"));
}

TEST_CASE("Parser fuzzy prefers enabled candidate over disabled")
{
    App app("test", "1.0", "Err msg");
    app.add_leaf("server", "Server");
    app.add_leaf("serve", "Serve").enabled([] { return false; });
    Argv argv{"test", "servr"};
    auto r = app.parse_fuzzy(argv.argc(), argv.argv());
    REQUIRE(r.is_ok());
    CHECK(r.unwrap().matched_command()->name() == "server");
}

TEST_CASE("Parser fuzzy miss with multiple disabled candidates is unknown")
{
    App app("test", "1.0", "Err msg");
    app.add_leaf("start", "Start").enabled([] { return false; });
    app.add_leaf("stop", "Stop").enabled([] { return false; });
    Argv argv{"test", "st"};
    auto r = app.parse_fuzzy(argv.argc(), argv.argv());
    CHECK(r.is_err());
    CHECK(std::holds_alternative<UnknownCommandError>(r.unwrap_err().info()));
    CHECK_FALSE(std::holds_alternative<CommandDisabledError>(r.unwrap_err().info()));
    CHECK(
        r.unwrap_err().what() == std::string_view("Parse Error: unknown command: 'st'"));
}

TEST_CASE("Parser fuzzy ambiguity ignores disabled candidates")
{
    App app("test", "1.0", "Err msg");
    app.add_leaf("start", "Start");
    app.add_leaf("stop", "Stop");
    app.add_leaf("stat", "Stat").enabled([] { return false; });
    Argv argv{"test", "st"};
    auto r = app.parse_fuzzy(argv.argc(), argv.argv());
    CHECK(r.is_err());
    auto &err = r.unwrap_err();
    REQUIRE(std::holds_alternative<AmbiguousCommandError>(err.info()));
    const auto &e = std::get<AmbiguousCommandError>(err.info());
    CHECK(
        std::find(e.candidates.begin(), e.candidates.end(), "stat") ==
        e.candidates.end());
}
