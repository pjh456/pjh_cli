#include <doctest/doctest.h>

#include <iostream>
#include <pjh_cli/app.hpp>
#include <pjh_cli/command/leaf_command.hpp>
#include <pjh_cli/core/fixed_string.hpp>
#include <pjh_cli/parse/parse_finalizer.hpp>
#include <pjh_cli/parse/parser.hpp>
#include <string_view>
#include <utility>

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
