#include <doctest/doctest.h>

#include <iostream>
#include <memory>
#include <pjh_cli/app.hpp>
#include <pjh_cli/console.hpp>
#include <pjh_cli/console/in_memory_history.hpp>
#include <stdexcept>
#include <string>
#include <string_view>

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

TEST_CASE("process_line suspends terminal around action")
{
    App app("test", "1.0", "Raw guard");
    int called = 0;
    auto &serve = app.add_leaf("serve", "Serve");
    serve.action(
        [&called](ParseContext &) -> CliResult<void>
        {
            ++called;
            return CliResult<void>::Ok();
        });

    StreamFixture streams;
    InteractiveConsole console(app, "> ", streams.input, streams.output, streams.error);
    auto term = std::make_unique<ScriptedTerminal>();
    ScriptedTerminal *term_ptr = term.get();
    console.set_terminal(std::move(term));

    auto r = console.process_line("serve");
    CHECK(r.is_ok());
    CHECK(called == 1);
    CHECK(term_ptr->suspend_calls == 1);
    CHECK(term_ptr->resume_calls == 1);
}

TEST_CASE("process_line resumes terminal when action throws")
{
    App app("test", "1.0", "Raw guard throw");
    auto &boom = app.add_leaf("boom", "Boom");
    boom.action(
        [](ParseContext &) -> CliResult<void> { throw std::runtime_error("boom"); });

    StreamFixture streams;
    InteractiveConsole console(app, "> ", streams.input, streams.output, streams.error);
    auto term = std::make_unique<ScriptedTerminal>();
    ScriptedTerminal *term_ptr = term.get();
    console.set_terminal(std::move(term));

    CHECK_THROWS_AS(
        [&console]
        {
            (void)console.process_line("boom");
        }(),
        std::runtime_error);
    CHECK(term_ptr->suspend_calls == 1);
    CHECK(term_ptr->resume_calls == 1);
}

TEST_CASE("set_terminal during run keeps the active terminal alive")
{
    App app("test", "1.0", "Terminal swap");
    InteractiveConsole console(app, "> ");
    auto t1 = std::make_unique<ScriptedTerminal>();
    std::shared_ptr<bool> t1_alive = t1->alive;
    auto t2 = std::make_unique<ScriptedTerminal>();
    ScriptedTerminal *p2 = t2.get();
    bool t1_alive_during_action = false;

    app.add_leaf("swap", "Swap")
        .action(
            [&](ParseContext &) -> CliResult<void>
            {
                console.set_terminal(std::move(t2));
                t1_alive_during_action = *t1_alive;
                return CliResult<void>::Ok();
            });

    for (char c : std::string("swap")) t1->keys.push_back({KeyEvent::Code::Character, c});
    t1->keys.push_back({KeyEvent::Code::Enter, 0});
    t1->keys.push_back({KeyEvent::Code::Eof, 0});
    t2->keys.push_back({KeyEvent::Code::Eof, 0});

    console.set_terminal(std::move(t1));
    console.run();

    CHECK(t1_alive_during_action);  // old terminal not freed by set_terminal
    CHECK(p2->read_calls >= 1u);    // replacement used on the next line
    CHECK_FALSE(*t1_alive);         // released after the in-flight line
}

TEST_CASE("nested run stop does not terminate the outer REPL")
{
    App app("test", "1.0", "Nested run getline");
    StreamFixture streams;
    InteractiveConsole console(app, "> ", streams.input, streams.output, streams.error);

    bool inner_entered = false;
    bool after_ran = false;

    auto &nest = app.add_leaf("nest", "Nest");
    nest.action(
        [&](ParseContext &) -> CliResult<void>
        {
            inner_entered = true;
            console.run();  // nested invocation on the same console
            return CliResult<void>::Ok();
        });
    auto &halt = app.add_leaf("halt", "Halt");
    halt.action(
        [&](ParseContext &) -> CliResult<void>
        {
            console.stop();  // clears the shared flag; must only stop the inner run
            return CliResult<void>::Ok();
        });
    auto &after = app.add_leaf("after", "After");
    after.action(
        [&](ParseContext &) -> CliResult<void>
        {
            after_ran = true;
            return CliResult<void>::Ok();
        });

    streams.input << "nest\nhalt\nafter\n";
    console.run();

    CHECK(inner_entered);
    CHECK(after_ran);
}

TEST_CASE("nested run on a scripted terminal does not terminate the outer REPL")
{
    App app("test", "1.0", "Nested run tty");
    InteractiveConsole console(app, "> ");

    bool after_ran = false;
    auto &nest = app.add_leaf("nest", "Nest");
    nest.action(
        [&](ParseContext &) -> CliResult<void>
        {
            console.run();
            return CliResult<void>::Ok();
        });
    auto &halt = app.add_leaf("halt", "Halt");
    halt.action(
        [&](ParseContext &) -> CliResult<void>
        {
            console.stop();
            return CliResult<void>::Ok();
        });
    auto &after = app.add_leaf("after", "After");
    after.action(
        [&](ParseContext &) -> CliResult<void>
        {
            after_ran = true;
            return CliResult<void>::Ok();
        });

    auto term = std::make_unique<ScriptedTerminal>();
    auto push = [&](std::string_view text)
    {
        for (char c : text) term->keys.push_back({KeyEvent::Code::Character, c});
        term->keys.push_back({KeyEvent::Code::Enter, 0});
    };
    push("nest");
    push("halt");
    push("after");
    term->keys.push_back({KeyEvent::Code::Eof, 0});
    console.set_terminal(std::move(term));

    console.run();

    CHECK(after_ran);
}

TEST_CASE("set_terminal during process_line keeps the guarded terminal alive")
{
    App app("test", "1.0", "Guard swap");
    InteractiveConsole console(app, "> ");
    auto t1 = std::make_unique<ScriptedTerminal>();
    std::shared_ptr<bool> t1_alive = t1->alive;
    auto t2 = std::make_unique<ScriptedTerminal>();
    bool t1_alive_during_action = false;

    app.add_leaf("swap", "Swap")
        .action(
            [&](ParseContext &) -> CliResult<void>
            {
                console.set_terminal(std::move(t2));
                t1_alive_during_action = *t1_alive;
                return CliResult<void>::Ok();
            });

    console.set_terminal(std::move(t1));
    auto r = console.process_line("swap");

    CHECK(r.is_ok());
    CHECK(t1_alive_during_action);
    CHECK_FALSE(*t1_alive);  // released when process_line returned
}

TEST_CASE("run default error formatter prints what")
{
    App app("test", "1.0", "Default err fmt");
    app.action(
        [](ParseContext &) -> CliResult<void>
        { return CliResult<void>::Err(ErrorFactory::runtime_error("boom")); });
    StreamFixture sf;
    sf.input << "anything\n";
    InteractiveConsole console(app, "> ", sf.input, sf.output, sf.error);
    console.run();
    CHECK(sf.error.str() == "boom\n");
}

TEST_CASE("run custom error formatter receives runtime kind")
{
    App app("test", "1.0", "Runtime err fmt");
    app.action(
        [](ParseContext &) -> CliResult<void>
        { return CliResult<void>::Err(ErrorFactory::runtime_error("boom")); });
    StreamFixture sf;
    sf.input << "anything\n";
    InteractiveConsole console(app, "> ", sf.input, sf.output, sf.error);
    ErrorKind seen = ErrorKind::Parse;
    int calls = 0;
    console.set_error_formatter(
        [&seen, &calls](const CliError &e)
        {
            ++calls;
            seen = e.kind();
            return std::string("ERR[") +
                   (e.kind() == ErrorKind::Runtime ? "rt" : "parse") + "]: " + e.what();
        });
    console.run();
    CHECK(calls == 1);
    CHECK(seen == ErrorKind::Runtime);
    CHECK(sf.error.str() == "ERR[rt]: boom\n");
}

TEST_CASE("run custom error formatter receives parse kind")
{
    App app("test", "1.0", "Parse err fmt");
    StreamFixture sf;
    sf.input << "--bogus\n";
    InteractiveConsole console(app, "> ", sf.input, sf.output, sf.error);
    ErrorKind seen = ErrorKind::Runtime;
    console.set_error_formatter(
        [&seen](const CliError &e)
        {
            seen = e.kind();
            return std::string("PARSE RENDERED");
        });
    console.run();
    CHECK(seen == ErrorKind::Parse);
    CHECK(sf.error.str() == "PARSE RENDERED\n");
}

TEST_CASE("run custom error formatter applies on terminal path")
{
    App app("test", "1.0", "Tty err fmt");
    StreamFixture sf;
    InteractiveConsole console(app, "> ", sf.input, sf.output, sf.error);
    console.set_error_formatter([](const CliError &) { return std::string("TTY ERR"); });
    auto term = std::make_unique<ScriptedTerminal>();
    for (char c : std::string("--bogus"))
        term->keys.push_back({KeyEvent::Code::Character, c});
    term->keys.push_back({KeyEvent::Code::Enter, 0});
    term->keys.push_back({KeyEvent::Code::Eof, 0});
    console.set_terminal(std::move(term));
    console.run();
    CHECK(sf.error.str() == "TTY ERR\n");
}

TEST_CASE("set_error_formatter empty restores default rendering")
{
    App app("test", "1.0", "Clear err fmt");
    app.action(
        [](ParseContext &) -> CliResult<void>
        { return CliResult<void>::Err(ErrorFactory::runtime_error("boom")); });
    StreamFixture sf;
    sf.input << "anything\n";
    InteractiveConsole console(app, "> ", sf.input, sf.output, sf.error);
    console.set_error_formatter([](const CliError &) { return std::string("CUSTOM"); });
    console.set_error_formatter({});
    console.run();
    CHECK(sf.error.str() == "boom\n");
}

TEST_CASE("error_formatter accessor reflects injection")
{
    App app("test", "1.0", "Err fmt accessor");
    InteractiveConsole console(app, "> ");
    CHECK_FALSE(console.error_formatter());
    console.set_error_formatter([](const CliError &) { return std::string("x"); });
    CHECK(console.error_formatter());
}

TEST_CASE("InteractiveConsole constructor accepts error formatter")
{
    App app("test", "1.0", "Ctor err fmt");
    StreamFixture sf;
    InteractiveConsole console(
        app, "> ", sf.input, sf.output, sf.error, {}, {}, nullptr,
        [](const CliError &) { return std::string("CTOR ERR"); });
    sf.input << "--bogus\n";
    console.run();
    CHECK(sf.error.str() == "CTOR ERR\n");
}
