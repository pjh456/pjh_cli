#include <doctest/doctest.h>

#include <iostream>
#include <memory>
#include <pjh_cli/app.hpp>
#include <pjh_cli/console.hpp>
#include <pjh_cli/console/in_memory_history.hpp>
#include <string>

#include "test_helpers.hpp"

using namespace pjh::cli;

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

TEST_CASE("InteractiveConsole Tab completion executes completed line")
{
    App app("test", "1.0", "Console completion");
    bool ran = false;
    auto &serve = app.add_leaf("serve", "Serve");
    serve.action(
        [&ran](ParseContext &) -> CliResult<void>
        {
            ran = true;
            return CliResult<void>::Ok();
        });

    StreamFixture streams;
    InteractiveConsole console(app, "> ", streams.input, streams.output, streams.error);

    auto term = std::make_unique<ScriptedTerminal>();
    ScriptedTerminal *term_ptr = term.get();
    for (char c : std::string("ser"))
        term->keys.push_back({KeyEvent::Code::Character, c});
    term->keys.push_back({KeyEvent::Code::Tab, 0});
    term->keys.push_back({KeyEvent::Code::Enter, 0});
    term->keys.push_back({KeyEvent::Code::Eof, 0});
    console.set_terminal(std::move(term));

    console.run();

    CHECK(ran);
    CHECK(term_ptr->written.find("serve ") != std::string::npos);
}

TEST_CASE("InteractiveConsole Up recalls and executes history")
{
    App app("test", "1.0", "History nav");
    int called = 0;
    app.action(
        [&called](ParseContext &) -> CliResult<void>
        {
            ++called;
            return CliResult<void>::Ok();
        });

    auto hist = std::make_unique<InMemoryHistory>();
    auto *raw = hist.get();
    raw->push("do-something");

    StreamFixture streams;
    InteractiveConsole console(
        app, "> ", streams.input, streams.output, streams.error, {}, {}, std::move(hist));

    auto term = std::make_unique<ScriptedTerminal>();
    term->keys.push_back({KeyEvent::Code::Up, 0});
    term->keys.push_back({KeyEvent::Code::Enter, 0});
    term->keys.push_back({KeyEvent::Code::Eof, 0});
    console.set_terminal(std::move(term));

    console.run();

    CHECK(called == 1);
    CHECK(raw->size() == 1);
}
