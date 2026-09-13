#include <doctest/doctest.h>

#include <iostream>
#include <pjh_cli/app.hpp>
#include <pjh_cli/console.hpp>
#include <pjh_cli/core/error.hpp>
#include <pjh_cli/core/fixed_string.hpp>
#include <pjh_cli/core/type.hpp>
#include <pjh_cli/parse/parse_context.hpp>
#include <string>
#include <string_view>
#include <variant>

using namespace pjh::cli;

TEST_CASE("process_line parse error returns err")
{
    App app("test", "1.0", "Parse err");
    InteractiveConsole console(app, "> ");

    auto r = console.process_line("--bogus");
    CHECK(r.is_err());
}

TEST_CASE("process_line invokes action on root")
{
    App app("test", "1.0", "Action");
    int called = 0;
    app.action(
        [&called](ParseContext &) -> CliResult<void>
        {
            ++called;
            return CliResult<void>::Ok();
        });

    InteractiveConsole console(app, "> ");
    auto r = console.process_line("somearg");
    CHECK(r.is_ok());
    CHECK(called == 1);
}

TEST_CASE("process_line subcommand action invoked")
{
    App app("test", "1.0", "Sub action");
    int called = 0;
    auto &serve = app.add_leaf("serve", "Serve");
    serve.action(
        [&called](ParseContext &) -> CliResult<void>
        {
            ++called;
            return CliResult<void>::Ok();
        });

    InteractiveConsole console(app, "> ");
    auto r = console.process_line("serve");
    CHECK(r.is_ok());
    CHECK(called == 1);
}

TEST_CASE("process_line with quoted string")
{
    App app("test", "1.0", "Quoted");
    auto &cmd = app.add_leaf("cmd", "Command");
    cmd.arg<std::string, 0>("file", "File");
    InteractiveConsole console(app, "> ");

    auto r = console.process_line(R"(cmd "my file.txt")");
    CHECK(r.is_ok());
}

TEST_CASE("process_line execution error propagates")
{
    App app("test", "1.0", "Exec err");
    app.action(
        [](ParseContext &) -> CliResult<void>
        { return CliResult<void>::Err(CliError("exec failed")); });

    InteractiveConsole console(app, "> ");
    auto r = console.process_line("anything");
    CHECK(r.is_err());
    auto &err = r.unwrap_err();
    CHECK(err.what() == std::string_view("exec failed"));
    CHECK(err.kind() == ErrorKind::Runtime);
}

TEST_CASE("process_line -- after subcommand")
{
    App app("test", "1.0", "Dash sub");
    auto &fmt = app.add_leaf("fmt", "Format");
    fmt.arg<std::string, 0>("file", "File");

    InteractiveConsole console(app, "> ");
    auto r = console.process_line("fmt -- --filename=bar");
    CHECK(r.is_ok());
}

TEST_CASE("process_line negative number positional after subcommand")
{
    App app("test", "1.0", "Neg sub");
    auto &run = app.add_leaf("run", "Run");
    run.arg<int, 0>("count", "Count");

    InteractiveConsole console(app, "> ");
    auto r = console.process_line("run -5");
    CHECK(r.is_ok());
}

TEST_CASE("process_line ancestor option after subcommand descent")
{
    App app("test", "1.0", "Ancestor repl");
    app.option<fixed_string("verbose")>("--verbose", 'v', "Verbose").boolean();
    app.add_leaf("son", "Son");

    InteractiveConsole console(app, "> ");
    auto r = console.process_line("son --verbose");
    CHECK(r.is_ok());
}

TEST_CASE("process_line ambiguous fuzzy returns ambiguous error")
{
    App app("test", "1.0", "Ambiguous repl");
    app.add_leaf("start", "Start");
    app.add_leaf("stop", "Stop");

    InteractiveConsole console(app, "> ");
    auto r = console.process_line("st");
    CHECK(r.is_err());
    auto &err = r.unwrap_err();
    CHECK(std::holds_alternative<AmbiguousCommandError>(err.info()));
    CHECK(
        std::string_view(err.what()).find("ambiguous command 'st'") !=
        std::string_view::npos);
}

TEST_CASE("process_line splits tab-separated tokens")
{
    App app("test", "1.0", "Tab tokens");
    bool called = false;
    bool verbose = false;
    auto &run = app.add_leaf("run", "Run");
    run.option<fixed_string("verbose")>("--verbose", 'v', "Verbose").boolean();
    run.action(
        [&](ParseContext &ctx) -> CliResult<void>
        {
            called = true;
            verbose = ctx.get<bool, fixed_string("verbose")>();
            return CliResult<void>::Ok();
        });

    InteractiveConsole console(app, "> ");
    auto r = console.process_line("run\t--verbose");
    CHECK(r.is_ok());
    CHECK(called);
    CHECK(verbose);
}

TEST_CASE("process_line empty quoted positional")
{
    App app("test", "1.0", "Empty positional");
    bool called = false;
    std::string captured = "sentinel";
    auto &cmd = app.add_leaf("cmd", "Command");
    cmd.arg<std::string, 0>("file", "File");
    cmd.action(
        [&](ParseContext &ctx) -> CliResult<void>
        {
            called = true;
            captured = ctx.get<std::string, 0>();
            return CliResult<void>::Ok();
        });

    InteractiveConsole console(app, "> ");
    auto r = console.process_line(R"(cmd "")");
    CHECK(r.is_ok());
    CHECK(called);
    CHECK(captured.empty());
}

TEST_CASE("process_line escaped quote positional")
{
    App app("test", "1.0", "Escaped quote");
    std::string captured;
    auto &cmd = app.add_leaf("cmd", "Command");
    cmd.arg<std::string, 0>("file", "File");
    cmd.action(
        [&](ParseContext &ctx) -> CliResult<void>
        {
            captured = ctx.get<std::string, 0>();
            return CliResult<void>::Ok();
        });

    InteractiveConsole console(app, "> ");
    auto r = console.process_line(R"(cmd "a\"b")");
    CHECK(r.is_ok());
    CHECK(captured == "a\"b");
}

TEST_CASE("process_line lone empty quote is an error on a dispatcher branch")
{
    App app("test", "1.0", "Empty quote dispatch");
    app.add_leaf("serve", "Serve");

    InteractiveConsole console(app, "> ");
    auto r = console.process_line(R"("")");
    CHECK(r.is_err());
}
