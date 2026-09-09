#include <doctest/doctest.h>

#include <iostream>
#include <memory>
#include <pjh_cli/app.hpp>
#include <pjh_cli/console.hpp>
#include <pjh_cli/console/history.hpp>
#include <pjh_cli/console/in_memory_history.hpp>
#include <pjh_cli/console/noop_history.hpp>
#include <pjh_cli/console/ring_buffer_history.hpp>
#include <pjh_result.hpp>
#include <sstream>
#include <string>
#include <string_view>

#include "test_helpers.hpp"

using namespace pjh::cli;
using Option = pjh::result::Option<std::string>;

// ── InMemoryHistory unit tests ──

TEST_CASE("InMemoryHistory empty")
{
    InMemoryHistory h;
    CHECK(h.size() == 0);
    CHECK(h.prev().is_none());
    CHECK(h.next().is_none());
}

TEST_CASE("InMemoryHistory push and prev")
{
    InMemoryHistory h;
    h.push("first");
    h.push("second");
    h.push("third");

    CHECK(h.size() == 3);

    auto v = h.prev();
    CHECK(v.is_some());
    CHECK(v.unwrap() == "third");

    v = h.prev();
    CHECK(v.is_some());
    CHECK(v.unwrap() == "second");

    v = h.prev();
    CHECK(v.is_some());
    CHECK(v.unwrap() == "first");

    v = h.prev();
    CHECK(v.is_none());
}

TEST_CASE("InMemoryHistory next after prev")
{
    InMemoryHistory h;
    h.push("a");
    h.push("b");
    h.push("c");

    (void)h.prev();  // c
    (void)h.prev();  // b
    (void)h.prev();  // a

    auto v = h.next();
    CHECK(v.is_some());
    CHECK(v.unwrap() == "b");

    v = h.next();
    CHECK(v.is_some());
    CHECK(v.unwrap() == "c");

    v = h.next();
    CHECK(v.is_none());

    v = h.next();
    CHECK(v.is_none());
}

TEST_CASE("InMemoryHistory consecutive duplicate dedup")
{
    InMemoryHistory h;
    h.push("hello");
    h.push("hello");
    h.push("world");
    h.push("world");

    CHECK(h.size() == 2);

    auto v = h.prev();
    CHECK(v.is_some());
    CHECK(v.unwrap() == "world");

    v = h.prev();
    CHECK(v.is_some());
    CHECK(v.unwrap() == "hello");
}

TEST_CASE("InMemoryHistory empty push ignored")
{
    InMemoryHistory h;
    h.push("");
    CHECK(h.size() == 0);
}

TEST_CASE("InMemoryHistory clear")
{
    InMemoryHistory h;
    h.push("a");
    h.push("b");
    CHECK(h.size() == 2);

    h.clear();
    CHECK(h.size() == 0);
    CHECK(h.prev().is_none());
}

TEST_CASE("InMemoryHistory reset_cursor")
{
    InMemoryHistory h;
    h.push("x");
    h.push("y");

    (void)h.prev();  // y
    (void)h.prev();  // x
    CHECK(h.prev().is_none());

    h.reset_cursor();
    auto v = h.prev();
    CHECK(v.is_some());
    CHECK(v.unwrap() == "y");
}

// ── Integration with InteractiveConsole ──

TEST_CASE("InteractiveConsole history pushed after execution")
{
    App app("test", "1.0", "History test");
    int called = 0;
    app.action(
        [&called](ParseContext &) -> CliResult<void>
        {
            ++called;
            return CliResult<void>::Ok();
        });

    std::stringstream input, output, error;
    auto hist = std::make_unique<InMemoryHistory>();
    auto *raw = hist.get();
    InteractiveConsole console(app, "> ", input, output, error, {}, {}, std::move(hist));

    auto r = console.process_line("do-something");
    CHECK(r.is_ok());
    CHECK(called == 1);
    CHECK(raw->size() == 1);
}

TEST_CASE("InteractiveConsole history pushed for help")
{
    App app("test", "1.0", "History help");
    app.add_leaf("serve", "Server");

    std::stringstream input, output, error;
    auto hist = std::make_unique<InMemoryHistory>();
    auto *raw = hist.get();
    InteractiveConsole console(app, "> ", input, output, error, {}, {}, std::move(hist));

    auto r = console.process_line("help");
    CHECK(r.is_ok());
    CHECK(raw->size() == 1);
    CHECK(raw->prev().unwrap() == "help");
}

TEST_CASE("InteractiveConsole history pushed for query")
{
    App app("test", "1.0", "History query");
    app.add_leaf("serve", "Server");

    std::stringstream input, output, error;
    auto hist = std::make_unique<InMemoryHistory>();
    auto *raw = hist.get();
    InteractiveConsole console(app, "> ", input, output, error, {}, {}, std::move(hist));

    auto r = console.process_line("?");
    CHECK(r.is_ok());
    CHECK(raw->size() == 1);
    CHECK(raw->prev().unwrap() == "?");
}

TEST_CASE("InteractiveConsole history pushed even on execution error")
{
    App app("test", "1.0", "History exec err");
    app.action(
        [](ParseContext &) -> CliResult<void>
        { return CliResult<void>::Err(CliError("fail")); });

    std::stringstream input, output, error;
    auto hist = std::make_unique<InMemoryHistory>();
    auto *raw = hist.get();
    InteractiveConsole console(app, "> ", input, output, error, {}, {}, std::move(hist));

    auto r = console.process_line("some-command");
    CHECK(r.is_err());
    CHECK(raw->size() == 1);
}

TEST_CASE("InteractiveConsole history pushed on parse error")
{
    App app("test", "1.0", "History parse error");
    std::stringstream input, output, error;
    auto hist = std::make_unique<InMemoryHistory>();
    auto *raw = hist.get();
    InteractiveConsole console(app, "> ", input, output, error, {}, {}, std::move(hist));

    auto r = console.process_line("--bogus");
    CHECK(r.is_err());
    CHECK(raw->size() == 1);
    CHECK(raw->prev().unwrap() == "--bogus");
}

TEST_CASE("InteractiveConsole history pushed for cmd --help")
{
    App app("test", "1.0", "History help_requested");
    app.add_leaf("serve", "Server");
    std::stringstream input, output, error;
    auto hist = std::make_unique<InMemoryHistory>();
    auto *raw = hist.get();
    InteractiveConsole console(app, "> ", input, output, error, {}, {}, std::move(hist));

    auto r = console.process_line("serve --help");
    CHECK(r.is_ok());
    CHECK(raw->size() == 1);
    CHECK(raw->prev().unwrap() == "serve --help");
}

TEST_CASE("InteractiveConsole history records whitespace-only line")
{
    App app("test", "1.0", "History whitespace");
    std::stringstream input, output, error;
    auto hist = std::make_unique<InMemoryHistory>();
    auto *raw = hist.get();
    InteractiveConsole console(app, "> ", input, output, error, {}, {}, std::move(hist));

    CHECK(console.process_line("   ").is_ok());
    CHECK(raw->size() == 1);
    CHECK(raw->prev().unwrap() == "   ");
}

TEST_CASE("InteractiveConsole history ignores empty line")
{
    App app("test", "1.0", "History empty");
    std::stringstream input, output, error;
    auto hist = std::make_unique<InMemoryHistory>();
    auto *raw = hist.get();
    InteractiveConsole console(app, "> ", input, output, error, {}, {}, std::move(hist));

    CHECK(console.process_line("").is_ok());
    CHECK(raw->size() == 0);
}

TEST_CASE("InteractiveConsole history dedups consecutive process_line duplicates")
{
    App app("test", "1.0", "History dedup");
    app.add_leaf("serve", "Server");
    std::stringstream input, output, error;
    auto hist = std::make_unique<InMemoryHistory>();
    auto *raw = hist.get();
    InteractiveConsole console(app, "> ", input, output, error, {}, {}, std::move(hist));

    CHECK(console.process_line("help").is_ok());
    CHECK(console.process_line("help").is_ok());
    CHECK(raw->size() == 1);
}

TEST_CASE("InteractiveConsole Up recalls a parse-failed line")
{
    App app("test", "1.0", "History recall parse");
    StreamFixture streams;
    auto hist = std::make_unique<InMemoryHistory>();
    auto *raw = hist.get();
    InteractiveConsole console(
        app, "> ", streams.input, streams.output, streams.error, {}, {}, std::move(hist));

    auto term = std::make_unique<ScriptedTerminal>();
    for (char c : std::string("--bogus"))
        term->keys.push_back({KeyEvent::Code::Character, c});
    term->keys.push_back({KeyEvent::Code::Enter, 0});
    term->keys.push_back({KeyEvent::Code::Up, 0});
    term->keys.push_back({KeyEvent::Code::Enter, 0});
    term->keys.push_back({KeyEvent::Code::Eof, 0});
    console.set_terminal(std::move(term));

    console.run();

    CHECK(raw->size() == 1);
    const std::string errors = streams.error.str();
    const auto first = errors.find("unknown option");
    REQUIRE(first != std::string::npos);
    // Second submission is the Up-recalled failed line -> the error appears twice.
    CHECK(errors.find("unknown option", first + 1) != std::string::npos);
}

TEST_CASE("InteractiveConsole with nullptr history disables push")
{
    App app("test", "1.0", "No history");
    int called = 0;
    app.action(
        [&called](ParseContext &) -> CliResult<void>
        {
            ++called;
            return CliResult<void>::Ok();
        });

    std::stringstream input, output, error;
    InteractiveConsole console(app, "> ", input, output, error, {}, {}, nullptr);

    auto r = console.process_line("something");
    CHECK(r.is_ok());
    CHECK(called == 1);
}

TEST_CASE("InteractiveConsole history persists across multiple lines")
{
    App app("test", "1.0", "Multi history");
    std::vector<std::string> calls;
    app.action(
        [&calls](ParseContext &) -> CliResult<void>
        {
            calls.push_back("ok");
            return CliResult<void>::Ok();
        });

    std::stringstream input, output, error;
    auto hist = std::make_unique<InMemoryHistory>();
    auto *raw = hist.get();
    InteractiveConsole console(app, "> ", input, output, error, {}, {}, std::move(hist));

    CHECK(console.process_line("first").is_ok());
    CHECK(console.process_line("second").is_ok());
    CHECK(console.process_line("third").is_ok());

    CHECK(raw->size() == 3);
    CHECK(calls.size() == 3);
}

// ── NoOpHistory tests ──

TEST_CASE("NoOpHistory always empty")
{
    NoOpHistory h;
    CHECK(h.size() == 0);
    CHECK(h.prev().is_none());
    CHECK(h.next().is_none());
}

TEST_CASE("NoOpHistory push does nothing")
{
    NoOpHistory h;
    h.push("anything");
    h.push("else");
    CHECK(h.size() == 0);
    CHECK(h.prev().is_none());
}

TEST_CASE("NoOpHistory clear is safe")
{
    NoOpHistory h;
    h.clear();
    CHECK(h.size() == 0);
}

TEST_CASE("NoOpHistory reset_cursor is safe")
{
    NoOpHistory h;
    h.reset_cursor();
    CHECK(h.prev().is_none());
}

TEST_CASE("InteractiveConsole with NoOpHistory")
{
    App app("test", "1.0", "NoOp");
    int called = 0;
    app.action(
        [&called](ParseContext &) -> CliResult<void>
        {
            ++called;
            return CliResult<void>::Ok();
        });

    std::stringstream input, output, error;
    InteractiveConsole console(app, "> ", input, output, error, {}, {},
                               std::make_unique<NoOpHistory>());

    auto r = console.process_line("cmd");
    CHECK(r.is_ok());
    CHECK(called == 1);
}

// ── RingBufferHistory tests ──

TEST_CASE("RingBufferHistory zero max_size throws")
{
    CHECK_THROWS_AS(RingBufferHistory(0), std::invalid_argument);
}

TEST_CASE("RingBufferHistory empty")
{
    RingBufferHistory h(5);
    CHECK(h.size() == 0);
    CHECK(h.prev().is_none());
    CHECK(h.next().is_none());
}

TEST_CASE("RingBufferHistory push less than capacity")
{
    RingBufferHistory h(5);
    h.push("a");
    h.push("b");
    h.push("c");
    CHECK(h.size() == 3);

    auto v = h.prev();
    CHECK(v.is_some());
    CHECK(v.unwrap() == "c");
}

TEST_CASE("RingBufferHistory push exactly capacity")
{
    RingBufferHistory h(3);
    h.push("a");
    h.push("b");
    h.push("c");
    CHECK(h.size() == 3);

    auto v = h.prev();
    CHECK(v.unwrap() == "c");
    v = h.prev();
    CHECK(v.unwrap() == "b");
    v = h.prev();
    CHECK(v.unwrap() == "a");
}

TEST_CASE("RingBufferHistory push exceeds capacity drops oldest")
{
    RingBufferHistory h(3);
    h.push("a");
    h.push("b");
    h.push("c");
    h.push("d");
    h.push("e");
    CHECK(h.size() == 3);

    // "a" and "b" should have been dropped
    auto v = h.prev();
    CHECK(v.unwrap() == "e");
    v = h.prev();
    CHECK(v.unwrap() == "d");
    v = h.prev();
    CHECK(v.unwrap() == "c");
    CHECK(h.prev().is_none());
}

TEST_CASE("RingBufferHistory cursor after push beyond capacity")
{
    RingBufferHistory h(2);
    h.push("a");
    h.push("b");
    h.push("c");
    CHECK(h.size() == 2);

    // "a" dropped, cursor reset to end
    auto v = h.prev();
    CHECK(v.unwrap() == "c");
    v = h.next();
    CHECK(v.is_none());
}

TEST_CASE("RingBufferHistory prev and next navigation")
{
    RingBufferHistory h(5);
    h.push("x");
    h.push("y");
    h.push("z");

    (void)h.prev();  // z
    (void)h.prev();  // y
    (void)h.prev();  // x

    // At oldest, next goes forward
    auto v = h.next();
    CHECK(v.is_some());
    CHECK(v.unwrap() == "y");

    v = h.next();
    CHECK(v.unwrap() == "z");

    v = h.next();
    CHECK(v.is_none());
}

TEST_CASE("RingBufferHistory dedup consecutive duplicates")
{
    RingBufferHistory h(5);
    h.push("dup");
    h.push("dup");
    h.push("unique");
    CHECK(h.size() == 2);
}

TEST_CASE("RingBufferHistory clear")
{
    RingBufferHistory h(5);
    h.push("a");
    h.push("b");
    h.clear();
    CHECK(h.size() == 0);
    CHECK(h.prev().is_none());
}

TEST_CASE("RingBufferHistory reset_cursor")
{
    RingBufferHistory h(5);
    h.push("a");
    h.push("b");

    (void)h.prev();
    (void)h.prev();
    CHECK(h.prev().is_none());

    h.reset_cursor();
    auto v = h.prev();
    CHECK(v.is_some());
    CHECK(v.unwrap() == "b");
}

TEST_CASE("InteractiveConsole with RingBufferHistory")
{
    App app("test", "1.0", "RingBuf");
    std::vector<std::string> calls;
    app.action(
        [&calls](ParseContext &) -> CliResult<void>
        {
            calls.push_back("ok");
            return CliResult<void>::Ok();
        });

    std::stringstream input, output, error;
    auto hist = std::make_unique<RingBufferHistory>(3);
    auto *raw = hist.get();
    InteractiveConsole console(app, "> ", input, output, error, {}, {},
                               std::move(hist));

    CHECK(console.process_line("a").is_ok());
    CHECK(console.process_line("b").is_ok());
    CHECK(console.process_line("c").is_ok());
    CHECK(console.process_line("d").is_ok());

    CHECK(raw->size() == 3);
    CHECK(calls.size() == 4);
}
