#include <doctest/doctest.h>

#include <string>
#include <string_view>

#include <pjh_cli/app.hpp>
#include <pjh_cli/command/base_command.hpp>
#include <pjh_cli/console.hpp>
#include "test_helpers.hpp"

using namespace pjh::cli;

TEST_CASE("process_line help")
{
    App app("test", "1.0", "Help test");
    app.add_leaf("serve", "Start the server");
    InteractiveConsole console(app, "> ");

    CoutCapture cap;
    auto r = console.process_line("help");
    CHECK(r.is_ok());
    CHECK(cap.str().find("Usage:") != std::string_view::npos);
    CHECK(cap.str().find("serve") != std::string_view::npos);
}

TEST_CASE("process_line --help")
{
    App app("test", "1.0", "Dash help");
    InteractiveConsole console(app, "> ");

    CoutCapture cap;
    auto r = console.process_line("--help");
    CHECK(r.is_ok());
    CHECK(cap.str().find("Usage:") != std::string_view::npos);
}

TEST_CASE("process_line -h")
{
    App app("test", "1.0", "Short help");
    InteractiveConsole console(app, "> ");

    CoutCapture cap;
    auto r = console.process_line("-h");
    CHECK(r.is_ok());
    CHECK(cap.str().find("Usage:") != std::string_view::npos);
}

TEST_CASE("process_line query list all")
{
    App app("test", "1.0", "Query all");
    app.add_leaf("foo", "Foo command");
    app.add_leaf("bar", "Bar command");
    InteractiveConsole console(app, "> ");

    CoutCapture cap;
    auto r = console.process_line("?");
    CHECK(r.is_ok());
    CHECK(cap.str().find("Subcommands:") != std::string_view::npos);
    CHECK(cap.str().find("foo") != std::string_view::npos);
    CHECK(cap.str().find("bar") != std::string_view::npos);
}

TEST_CASE("process_line query substring match")
{
    App app("test", "1.0", "Query substring");
    app.add_leaf("server", "Server");
    app.add_leaf("config", "Config");
    InteractiveConsole console(app, "> ");

    CoutCapture cap;
    auto r = console.process_line("?serv");
    CHECK(r.is_ok());
    CHECK(cap.str().find("Matching subcommands:") != std::string_view::npos);
    CHECK(cap.str().find("server") != std::string_view::npos);
}

TEST_CASE("process_line query fuzzy fallback")
{
    App app("test", "1.0", "Query fuzzy");
    app.add_leaf("server", "Server command");
    InteractiveConsole console(app, "> ");

    CoutCapture cap;
    auto r = console.process_line("?servr");
    CHECK(r.is_ok());
    CHECK(cap.str().find("Did you mean:") != std::string_view::npos);
    CHECK(cap.str().find("server") != std::string_view::npos);
}

TEST_CASE("process_line query no match")
{
    App app("test", "1.0", "Query no match");
    app.add_leaf("server", "Server command");
    InteractiveConsole console(app, "> ");

    CoutCapture cap;
    auto r = console.process_line("?zzzzz");
    CHECK(r.is_ok());
    CHECK(cap.str().find("No matches.") != std::string_view::npos);
}

TEST_CASE("process_line query empty subcommands")
{
    App app("test", "1.0", "Query empty");
    InteractiveConsole console(app, "> ");

    CoutCapture cap;
    auto r = console.process_line("?");
    CHECK(r.is_ok());
    CHECK(cap.str().find("No subcommands available.") != std::string_view::npos);
}

TEST_CASE("process_line hidden subcommand not listed in query")
{
    App app("test", "1.0", "Hidden");
    app.add_leaf("visible", "Visible");
    app.add_leaf("hidden", "Hidden").set_visibility(Visibility::Hidden);

    CoutCapture cap;
    InteractiveConsole console(app, "> ");
    auto r = console.process_line("?");
    CHECK(r.is_ok());
    CHECK(cap.str().find("visible") != std::string_view::npos);
    CHECK(cap.str().find("hidden") == std::string_view::npos);
}

TEST_CASE("process_line disabled subcommand not listed in query")
{
    App app("test", "1.0", "Disabled");
    app.add_leaf("active", "Active");
    app.add_leaf("inactive", "Inactive").enabled([] { return false; });

    CoutCapture cap;
    InteractiveConsole console(app, "> ");
    auto r = console.process_line("?");
    CHECK(r.is_ok());
    CHECK(cap.str().find("active") != std::string_view::npos);
    CHECK(cap.str().find("inactive") == std::string_view::npos);
}
