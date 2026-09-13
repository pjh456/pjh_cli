#include <doctest/doctest.h>

#include <iostream>
#include <pjh_cli/app.hpp>
#include <pjh_cli/command/base_command.hpp>
#include <pjh_cli/console.hpp>
#include <pjh_cli/core/fixed_string.hpp>
#include <string>
#include <string_view>

#include "test_helpers.hpp"

using namespace pjh::cli;

TEST_CASE("process_line help")
{
    App app("test", "1.0", "Help test");
    app.add_leaf("serve", "Start the server");
    StreamFixture sf;
    InteractiveConsole console(app, "> ", sf.input, sf.output, sf.error);

    auto r = console.process_line("help");
    CHECK(r.is_ok());
    CHECK(sf.output.str().find("Usage:") != std::string_view::npos);
    CHECK(sf.output.str().find("serve") != std::string_view::npos);
}

TEST_CASE("process_line --help")
{
    App app("test", "1.0", "Dash help");
    StreamFixture sf;
    InteractiveConsole console(app, "> ", sf.input, sf.output, sf.error);

    auto r = console.process_line("--help");
    CHECK(r.is_ok());
    CHECK(sf.output.str().find("Usage:") != std::string_view::npos);
}

TEST_CASE("process_line -h")
{
    App app("test", "1.0", "Short help");
    StreamFixture sf;
    InteractiveConsole console(app, "> ", sf.input, sf.output, sf.error);

    auto r = console.process_line("-h");
    CHECK(r.is_ok());
    CHECK(sf.output.str().find("Usage:") != std::string_view::npos);
}

TEST_CASE("process_line help subcommand shows full path")
{
    App app("test", "1.0", "Repl help path");
    auto &container = app.add_branch("container", "Container");
    container.add_leaf("start", "Start");
    StreamFixture sf;
    InteractiveConsole console(app, "> ", sf.input, sf.output, sf.error);

    auto r = console.process_line("help container start");
    CHECK(r.is_ok());
    CHECK(sf.output.str().starts_with("Usage: test container start"));
}

TEST_CASE("process_line help shows option metadata annotations")
{
    App app("test", "1.0", "Repl metadata help");
    app.option<fixed_string("host")>("--host", "Host").str().env("APP_HOST");
    StreamFixture sf;
    InteractiveConsole console(app, "> ", sf.input, sf.output, sf.error);

    auto r = console.process_line("help");
    CHECK(r.is_ok());
    CHECK(sf.output.str().find("(env: APP_HOST)") != std::string_view::npos);
}

TEST_CASE("process_line subcommand --help shows full path")
{
    App app("test", "1.0", "Repl parse help path");
    auto &container = app.add_branch("container", "Container");
    container.add_leaf("start", "Start");
    StreamFixture sf;
    InteractiveConsole console(app, "> ", sf.input, sf.output, sf.error);

    auto r = console.process_line("container start --help");
    CHECK(r.is_ok());
    CHECK(sf.output.str().starts_with("Usage: test container start"));
}

TEST_CASE("process_line subcommand help shows inherited options")
{
    App app("test", "1.0", "Repl inherited help");
    app.option<fixed_string("verbose")>("--verbose", 'v', "Verbose").boolean();
    auto &container = app.add_branch("container", "Container");
    container.add_leaf("start", "Start");
    StreamFixture sf;
    InteractiveConsole console(app, "> ", sf.input, sf.output, sf.error);

    auto r = console.process_line("help container start");
    CHECK(r.is_ok());
    CHECK(sf.output.str().find("Inherited Options:") != std::string_view::npos);
    CHECK(sf.output.str().find("--verbose") != std::string_view::npos);
}

TEST_CASE("process_line query list all")
{
    App app("test", "1.0", "Query all");
    app.add_leaf("foo", "Foo command");
    app.add_leaf("bar", "Bar command");
    StreamFixture sf;
    InteractiveConsole console(app, "> ", sf.input, sf.output, sf.error);

    auto r = console.process_line("?");
    CHECK(r.is_ok());
    CHECK(sf.output.str().find("Subcommands:") != std::string_view::npos);
    CHECK(sf.output.str().find("foo") != std::string_view::npos);
    CHECK(sf.output.str().find("bar") != std::string_view::npos);
}

TEST_CASE("process_line query substring match")
{
    App app("test", "1.0", "Query substring");
    app.add_leaf("server", "Server");
    app.add_leaf("config", "Config");
    StreamFixture sf;
    InteractiveConsole console(app, "> ", sf.input, sf.output, sf.error);

    auto r = console.process_line("?serv");
    CHECK(r.is_ok());
    CHECK(sf.output.str().find("Matching subcommands:") != std::string_view::npos);
    CHECK(sf.output.str().find("server") != std::string_view::npos);
}

TEST_CASE("process_line query with padding stays a substring match")
{
    App app("test", "1.0", "Query padded");
    app.add_leaf("server", "Server");
    app.add_leaf("config", "Config");
    StreamFixture sf;
    InteractiveConsole console(app, "> ", sf.input, sf.output, sf.error);

    // `? <keyword>` is the documented padded form; it must not degrade to the
    // fuzzy "Did you mean" fallback just because of the space.
    auto spaced = console.process_line("? serv");
    CHECK(spaced.is_ok());
    CHECK(sf.output.str().find("Matching subcommands:") != std::string_view::npos);
    CHECK(sf.output.str().find("server") != std::string_view::npos);
    CHECK(sf.output.str().find("Did you mean:") == std::string_view::npos);

    sf.output.str("");
    sf.output.clear();
    auto left_padded = console.process_line("  ?serv");
    CHECK(left_padded.is_ok());
    CHECK(sf.output.str().find("Matching subcommands:") != std::string_view::npos);
    CHECK(sf.output.str().find("Did you mean:") == std::string_view::npos);

    sf.output.str("");
    sf.output.clear();
    auto right_padded = console.process_line("?serv ");
    CHECK(right_padded.is_ok());
    CHECK(sf.output.str().find("Matching subcommands:") != std::string_view::npos);
    CHECK(sf.output.str().find("Did you mean:") == std::string_view::npos);
}

TEST_CASE("process_line query fuzzy fallback")
{
    App app("test", "1.0", "Query fuzzy");
    app.add_leaf("server", "Server command");
    StreamFixture sf;
    InteractiveConsole console(app, "> ", sf.input, sf.output, sf.error);

    auto r = console.process_line("?servr");
    CHECK(r.is_ok());
    CHECK(sf.output.str().find("Did you mean:") != std::string_view::npos);
    CHECK(sf.output.str().find("server") != std::string_view::npos);
}

TEST_CASE("process_line query no match")
{
    App app("test", "1.0", "Query no match");
    app.add_leaf("server", "Server command");
    StreamFixture sf;
    InteractiveConsole console(app, "> ", sf.input, sf.output, sf.error);

    auto r = console.process_line("?zzzzz");
    CHECK(r.is_ok());
    CHECK(sf.output.str().find("No matches.") != std::string_view::npos);
}

TEST_CASE("process_line query empty subcommands")
{
    App app("test", "1.0", "Query empty");
    StreamFixture sf;
    InteractiveConsole console(app, "> ", sf.input, sf.output, sf.error);

    auto r = console.process_line("?");
    CHECK(r.is_ok());
    CHECK(sf.output.str().find("No subcommands available.") != std::string_view::npos);
}

TEST_CASE("process_line hidden subcommand not listed in query")
{
    App app("test", "1.0", "Hidden");
    app.add_leaf("visible", "Visible");
    app.add_leaf("hidden", "Hidden").set_visibility(Visibility::Hidden);

    StreamFixture sf;
    InteractiveConsole console(app, "> ", sf.input, sf.output, sf.error);
    auto r = console.process_line("?");
    CHECK(r.is_ok());
    CHECK(sf.output.str().find("visible") != std::string_view::npos);
    CHECK(sf.output.str().find("hidden") == std::string_view::npos);
}

TEST_CASE("process_line disabled subcommand not listed in query")
{
    App app("test", "1.0", "Disabled");
    app.add_leaf("active", "Active");
    app.add_leaf("inactive", "Inactive").enabled([] { return false; });

    StreamFixture sf;
    InteractiveConsole console(app, "> ", sf.input, sf.output, sf.error);
    auto r = console.process_line("?");
    CHECK(r.is_ok());
    CHECK(sf.output.str().find("active") != std::string_view::npos);
    CHECK(sf.output.str().find("inactive") == std::string_view::npos);
}

TEST_CASE("process_line subcommand --help uses App help formatter")
{
    App app("test", "1.0", "Repl injected help");
    auto &container = app.add_branch("container", "Container");
    container.add_leaf("start", "Start");
    std::string observed;
    app.set_help_formatter(
        [&observed](const BaseCommand &cmd)
        {
            observed = cmd.name();
            return std::string("REPL CUSTOM");
        });
    StreamFixture sf;
    InteractiveConsole console(app, "> ", sf.input, sf.output, sf.error);

    auto r = console.process_line("container start --help");
    CHECK(r.is_ok());
    CHECK(sf.output.str() == "REPL CUSTOM\n");  // process_line appends "\n"
    CHECK(observed == "start");                 // deepest command is passed
}

TEST_CASE("process_line leaf --help uses App help formatter")
{
    App app("test", "1.0", "Repl leaf injected help");
    app.add_leaf("serve", "Start");
    app.set_help_formatter([](const BaseCommand &)
                           { return std::string("LEAF CUSTOM"); });
    StreamFixture sf;
    InteractiveConsole console(app, "> ", sf.input, sf.output, sf.error);

    auto r = console.process_line("serve --help");
    CHECK(r.is_ok());
    CHECK(sf.output.str() == "LEAF CUSTOM\n");
}

TEST_CASE("process_line --help skips the root action with an empty formatter")
{
    App app("test", "1.0", "Repl root empty formatter");
    int called = 0;
    app.action(
        [&called](ParseContext &) -> CliResult<void>
        {
            ++called;
            return CliResult<void>::Ok();
        });
    app.set_help_formatter([](const BaseCommand &) { return std::string{}; });
    StreamFixture sf;
    InteractiveConsole console(app, "> ", sf.input, sf.output, sf.error);

    auto r = console.process_line("--help");
    CHECK(r.is_ok());
    CHECK(called == 0);
    CHECK(sf.output.str().starts_with("Usage: test"));
}

TEST_CASE("process_line subcommand empty help formatter falls back and skips the action")
{
    App app("test", "1.0", "Repl empty formatter");
    int called = 0;
    app.add_leaf("serve", "Start server")
        .action(
            [&called](ParseContext &) -> CliResult<void>
            {
                ++called;
                return CliResult<void>::Ok();
            });
    app.set_help_formatter([](const BaseCommand &) { return std::string{}; });
    StreamFixture sf;
    InteractiveConsole console(app, "> ", sf.input, sf.output, sf.error);

    auto r = console.process_line("serve --help");
    CHECK(r.is_ok());
    CHECK(called == 0);
    CHECK(sf.output.str().starts_with("Usage: test serve"));  // process_line adds "\n"
}

TEST_CASE("process_line help navigation keeps console formatter")
{
    App app("test", "1.0", "Repl formatter separation");
    app.add_leaf("serve", "Start");
    app.set_help_formatter([](const BaseCommand &)
                           { return std::string("BATCH CUSTOM"); });
    StreamFixture sf;
    InteractiveConsole console(app, "> ", sf.input, sf.output, sf.error);

    auto r = console.process_line("help serve");
    CHECK(r.is_ok());
    CHECK(sf.output.str().find("Usage:") != std::string_view::npos);
    CHECK(sf.output.str().find("BATCH CUSTOM") == std::string_view::npos);
}

TEST_CASE("process_line clearing App formatter restores built-in help")
{
    App app("test", "1.0", "Repl clear formatter");
    app.add_leaf("serve", "Start");
    app.set_help_formatter([](const BaseCommand &) { return std::string("CUSTOM"); });

    StreamFixture first;
    InteractiveConsole console(app, "> ", first.input, first.output, first.error);
    {
        auto r = console.process_line("serve --help");
        CHECK(r.is_ok());
        CHECK(first.output.str() == "CUSTOM\n");
    }

    app.set_help_formatter({});
    StreamFixture second;
    InteractiveConsole console2(app, "> ", second.input, second.output, second.error);
    auto r = console2.process_line("serve --help");
    CHECK(r.is_ok());
    CHECK(second.output.str().starts_with("Usage: test serve"));
}

TEST_CASE("process_line non-App branch root uses built-in help")
{
    BranchCommand root("test", "Plain branch");
    root.add_leaf("serve", "Start");
    StreamFixture sf;
    InteractiveConsole console(root, "> ", sf.input, sf.output, sf.error);

    auto r = console.process_line("serve --help");
    CHECK(r.is_ok());
    CHECK(sf.output.str().starts_with("Usage: test serve"));
}

TEST_CASE("process_line --version prints version")
{
    App app("test", "1.0", "Version repl");
    StreamFixture sf;
    InteractiveConsole console(app, "> ", sf.input, sf.output, sf.error);

    auto r = console.process_line("--version");
    CHECK(r.is_ok());
    // version_text() is newline-terminated: exactly one newline, no blank line.
    CHECK(sf.output.str() == "test version 1.0\n");
}

TEST_CASE("process_line --version does not execute the root action")
{
    App app("test", "1.0", "Version no exec");
    int called = 0;
    app.action(
        [&called](ParseContext &) -> CliResult<void>
        {
            ++called;
            return CliResult<void>::Ok();
        });
    StreamFixture sf;
    InteractiveConsole console(app, "> ", sf.input, sf.output, sf.error);

    auto r = console.process_line("--version");
    CHECK(r.is_ok());
    CHECK(called == 0);
    CHECK(sf.output.str() == "test version 1.0\n");
}

TEST_CASE("process_line subcommand --version prints root version")
{
    App app("test", "1.0", "Version sub repl");
    int called = 0;
    auto &serve = app.add_leaf("serve", "Serve");
    serve.action(
        [&called](ParseContext &) -> CliResult<void>
        {
            ++called;
            return CliResult<void>::Ok();
        });
    StreamFixture sf;
    InteractiveConsole console(app, "> ", sf.input, sf.output, sf.error);

    auto r = console.process_line("serve --version");
    CHECK(r.is_ok());
    CHECK(called == 0);                              // subcommand action skipped
    CHECK(sf.output.str() == "test version 1.0\n");  // root name/version
}

TEST_CASE("process_line --version ignores help and error formatters")
{
    App app("test", "1.0", "Version formatter separation");
    app.set_help_formatter([](const BaseCommand &)
                           { return std::string("BATCH CUSTOM"); });
    StreamFixture sf;
    InteractiveConsole console(app, "> ", sf.input, sf.output, sf.error);
    int error_calls = 0;
    console.set_error_formatter(
        [&error_calls](const CliError &)
        {
            ++error_calls;
            return std::string("ERR");
        });

    auto r = console.process_line("--version");
    CHECK(r.is_ok());
    CHECK(sf.output.str() == "test version 1.0\n");
    CHECK(sf.output.str().find("BATCH CUSTOM") == std::string::npos);
    CHECK(error_calls == 0);
    CHECK(sf.error.str().empty());
}

TEST_CASE("process_line --version after double dash is not consumed")
{
    App app("test", "1.0", "Version dash barrier");
    auto &run = app.add_leaf("run", "Run");
    run.arg<std::string, 0>("file", "File");
    StreamFixture sf;
    InteractiveConsole console(app, "> ", sf.input, sf.output, sf.error);

    auto r = console.process_line("run -- --version");
    CHECK(r.is_ok());
    CHECK(sf.output.str().empty());  // no version printed; arg consumed
}
