#include <pjh_cli.hpp>
#include <cassert>
#include <iostream>
#include <sstream>
#include <string>

using namespace pjh::cli;

int main()
{
    // ── edit_distance still works after refactor ──
    assert(edit_distance("", "") == 0);
    assert(edit_distance("abc", "abc") == 0);
    assert(edit_distance("abc", "abd") == 1);

    // ── parse_command free function ──
    {
        App app("test", "1.0", "Free function parse");
        app.option<int, fixed_string("port")>("--port", 'p', "Port");

        std::vector<std::string_view> args{"--port", "9090"};
        auto r = parse_command(app, args);
        assert(r.is_ok());
        auto val = r.unwrap().get<int, fixed_string("port")>();
        assert(val == 9090);
    }

    // ── parse_command_fuzzy free function ──
    {
        App app("test", "1.0", "Free function fuzzy");
        app.add_command("server", "Server");
        app.add_command("config", "Config");

        std::vector<std::string_view> args{"servr"};
        auto r = parse_command(app, args, 3);
        assert(r.is_ok());
        assert(r.unwrap().matched_path() == "server");
    }

    // ── execute() is now const ──
    {
        App app("test", "1.0", "Const execute");
        int called = 0;
        app.action(
            [&called](ParseContext &)
                -> CliResult<void>
            {
            ++called;
            return CliResult<void>::Ok(); });

        auto ctx = app.create_context();
        const App &const_app = app;
        auto r = const_app.execute(ctx);
        assert(r.is_ok());
        assert(called == 1);
    }

    // ── help text includes description ──
    {
        App app("test", "1.0.0", "A test application");
        auto help = format_help(app, "test");
        assert(help.find("A test application") != std::string_view::npos);
        assert(help.find("Usage:") != std::string_view::npos);
        assert(help.find("test") != std::string_view::npos);
    }

    // ── usage text includes program name ──
    {
        App app("myapp", "1.0", "My app");
        auto usage = format_usage(app, "myapp");
        assert(usage.find("Usage:") != std::string_view::npos);
        assert(usage.find("myapp") != std::string_view::npos);
    }

    // ── InteractiveConsole can be constructed ──
    {
        App app("test", "1.0", "Console test");
        InteractiveConsole console(app, ">");
        assert(console.prompt() == ">");
        console.set_prompt("$ ");
        assert(console.prompt() == "$ ");
    }

    return 0;
}
