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
    CHECK(r.unwrap_err().what() == std::string_view("Parse Error: exec failed"));
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
