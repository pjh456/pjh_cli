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
