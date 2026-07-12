#include <pjh_cli.hpp>
#include <doctest/doctest.h>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>

using namespace pjh::cli;

TEST_CASE("edit_distance after refactor")
{
    CHECK(edit_distance("", "") == 0);
    CHECK(edit_distance("abc", "abc") == 0);
    CHECK(edit_distance("abc", "abd") == 1);
}

TEST_CASE("parse_command free function")
{
    App app("test", "1.0", "Free function parse");
    app.option<int, fixed_string("port")>("--port", 'p', "Port");

    std::vector<std::string_view> args{"--port", "9090"};
    auto r = parse_command(app, args);
    CHECK(r.is_ok());
    auto val = r.unwrap().get<int, fixed_string("port")>();
    CHECK(val == 9090);
}

TEST_CASE("parse_command fuzzy free function")
{
    App app("test", "1.0", "Free function fuzzy");
    app.add_command("server", "Server");
    app.add_command("config", "Config");

    std::vector<std::string_view> args{"servr"};
    auto r = parse_command(app, args, 3);
    CHECK(r.is_ok());
    CHECK(r.unwrap().matched_path() == "server");
}

TEST_CASE("const execute")
{
    App app("test", "1.0", "Const execute");
    int called = 0;
    app.action([&called](ParseContext &) -> CliResult<void> {
        ++called;
        return CliResult<void>::Ok();
    });

    auto ctx = app.create_context();
    const App &const_app = app;
    auto r = const_app.execute(ctx);
    CHECK(r.is_ok());
    CHECK(called == 1);
}

TEST_CASE("format help includes description")
{
    App app("test", "1.0.0", "A test application");
    auto help = format_help(app, "test");
    CHECK(help.find("A test application") != std::string_view::npos);
    CHECK(help.find("Usage:") != std::string_view::npos);
    CHECK(help.find("test") != std::string_view::npos);
}

TEST_CASE("format usage includes program name")
{
    App app("myapp", "1.0", "My app");
    auto usage = format_usage(app, "myapp");
    CHECK(usage.find("Usage:") != std::string_view::npos);
    CHECK(usage.find("myapp") != std::string_view::npos);
}

TEST_CASE("InteractiveConsole construction")
{
    App app("test", "1.0", "Console test");
    InteractiveConsole console(app, ">");
    CHECK(console.prompt() == ">");
    console.set_prompt("$ ");
    CHECK(console.prompt() == "$ ");
}

TEST_CASE("InteractiveConsole stop")
{
    App app("test", "1.0", "Stop test");
    InteractiveConsole console(app, "> ");
    console.stop();
}

TEST_CASE("process_line empty string")
{
    App app("test", "1.0", "Empty line");
    InteractiveConsole console(app, "> ");
    auto r = console.process_line("");
    CHECK(r.is_ok());
}

TEST_CASE("process_line whitespace only")
{
    App app("test", "1.0", "Whitespace");
    InteractiveConsole console(app, "> ");
    auto r = console.process_line("   ");
    CHECK(r.is_ok());
}

TEST_CASE("process_line help")
{
    App app("test", "1.0", "Help test");
    app.add_command("serve", "Start the server");
    InteractiveConsole console(app, "> ");

    std::stringstream buf;
    auto old = std::cout.rdbuf(buf.rdbuf());

    auto r = console.process_line("help");
    std::cout.rdbuf(old);
    CHECK(r.is_ok());
    CHECK(buf.str().find("Usage:") != std::string_view::npos);
    CHECK(buf.str().find("serve") != std::string_view::npos);
}

TEST_CASE("process_line --help")
{
    App app("test", "1.0", "Dash help");
    InteractiveConsole console(app, "> ");

    std::stringstream buf;
    auto old = std::cout.rdbuf(buf.rdbuf());

    auto r = console.process_line("--help");
    std::cout.rdbuf(old);
    CHECK(r.is_ok());
    CHECK(buf.str().find("Usage:") != std::string_view::npos);
}

TEST_CASE("process_line -h")
{
    App app("test", "1.0", "Short help");
    InteractiveConsole console(app, "> ");

    std::stringstream buf;
    auto old = std::cout.rdbuf(buf.rdbuf());

    auto r = console.process_line("-h");
    std::cout.rdbuf(old);
    CHECK(r.is_ok());
    CHECK(buf.str().find("Usage:") != std::string_view::npos);
}

TEST_CASE("process_line query list all")
{
    App app("test", "1.0", "Query all");
    app.add_command("foo", "Foo command");
    app.add_command("bar", "Bar command");
    InteractiveConsole console(app, "> ");

    std::stringstream buf;
    auto old = std::cout.rdbuf(buf.rdbuf());

    auto r = console.process_line("?");
    std::cout.rdbuf(old);
    CHECK(r.is_ok());
    CHECK(buf.str().find("Subcommands:") != std::string_view::npos);
    CHECK(buf.str().find("foo") != std::string_view::npos);
    CHECK(buf.str().find("bar") != std::string_view::npos);
}

TEST_CASE("process_line query substring match")
{
    App app("test", "1.0", "Query substring");
    app.add_command("server", "Server");
    app.add_command("config", "Config");
    InteractiveConsole console(app, "> ");

    std::stringstream buf;
    auto old = std::cout.rdbuf(buf.rdbuf());

    auto r = console.process_line("?serv");
    std::cout.rdbuf(old);
    CHECK(r.is_ok());
    CHECK(buf.str().find("Matching subcommands:") != std::string_view::npos);
    CHECK(buf.str().find("server") != std::string_view::npos);
}

TEST_CASE("process_line query fuzzy fallback")
{
    App app("test", "1.0", "Query fuzzy");
    app.add_command("server", "Server command");
    InteractiveConsole console(app, "> ");

    std::stringstream buf;
    auto old = std::cout.rdbuf(buf.rdbuf());

    auto r = console.process_line("?servr");
    std::cout.rdbuf(old);
    CHECK(r.is_ok());
    CHECK(buf.str().find("Did you mean:") != std::string_view::npos);
    CHECK(buf.str().find("server") != std::string_view::npos);
}

TEST_CASE("process_line query no match")
{
    App app("test", "1.0", "Query no match");
    app.add_command("server", "Server command");
    InteractiveConsole console(app, "> ");

    std::stringstream buf;
    auto old = std::cout.rdbuf(buf.rdbuf());

    auto r = console.process_line("?zzzzz");
    std::cout.rdbuf(old);
    CHECK(r.is_ok());
    CHECK(buf.str().find("No matches.") != std::string_view::npos);
}

TEST_CASE("process_line query empty subcommands")
{
    App app("test", "1.0", "Query empty");
    InteractiveConsole console(app, "> ");

    std::stringstream buf;
    auto old = std::cout.rdbuf(buf.rdbuf());

    auto r = console.process_line("?");
    std::cout.rdbuf(old);
    CHECK(r.is_ok());
    CHECK(buf.str().find("No subcommands available.") != std::string_view::npos);
}

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
    app.action([&called](ParseContext &) -> CliResult<void> {
        ++called;
        return CliResult<void>::Ok();
    });

    InteractiveConsole console(app, "> ");
    // Any non-special token will parse as positional, then execute root action
    auto r = console.process_line("somearg");
    CHECK(r.is_ok());
    CHECK(called == 1);
}

TEST_CASE("process_line subcommand action invoked")
{
    App app("test", "1.0", "Sub action");
    int called = 0;
    auto &serve = app.add_command("serve", "Serve");
    serve.action([&called](ParseContext &) -> CliResult<void> {
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
    app.arg<std::string, 0>("file", "File");
    InteractiveConsole console(app, "> ");

    auto r = console.process_line(R"( "my file.txt" )");
    CHECK(r.is_ok());
}

TEST_CASE("process_line execution error propagates")
{
    App app("test", "1.0", "Exec err");
    app.action([](ParseContext &) -> CliResult<void> {
        return CliResult<void>::Err(CliError("exec failed"));
    });

    InteractiveConsole console(app, "> ");
    auto r = console.process_line("anything");
    CHECK(r.is_err());
    CHECK(r.unwrap_err().what() == std::string_view("Parse Error: exec failed"));
}

TEST_CASE("process_line -- after subcommand")
{
    App app("test", "1.0", "Dash sub");
    auto &fmt = app.add_command("fmt", "Format");
    fmt.arg<std::string, 0>("file", "File");

    InteractiveConsole console(app, "> ");
    auto r = console.process_line("fmt -- --filename=bar");
    CHECK(r.is_ok());
}

TEST_CASE("process_line hidden subcommand not listed in query")
{
    App app("test", "1.0", "Hidden");
    app.add_command("visible", "Visible");
    app.add_command("hidden", "Hidden").set_visibility(Visibility::Hidden);

    std::stringstream buf;
    auto old = std::cout.rdbuf(buf.rdbuf());

    InteractiveConsole console(app, "> ");
    auto r = console.process_line("?");
    std::cout.rdbuf(old);
    CHECK(r.is_ok());
    CHECK(buf.str().find("visible") != std::string_view::npos);
    CHECK(buf.str().find("hidden") == std::string_view::npos);
}

TEST_CASE("process_line disabled subcommand not listed in query")
{
    App app("test", "1.0", "Disabled");
    app.add_command("active", "Active");
    app.add_command("inactive", "Inactive").enabled([] { return false; });

    std::stringstream buf;
    auto old = std::cout.rdbuf(buf.rdbuf());

    InteractiveConsole console(app, "> ");
    auto r = console.process_line("?");
    std::cout.rdbuf(old);
    CHECK(r.is_ok());
    CHECK(buf.str().find("active") != std::string_view::npos);
    CHECK(buf.str().find("inactive") == std::string_view::npos);
}
