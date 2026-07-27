#include <doctest/doctest.h>

#include <iostream>
#include <pjh_cli/app.hpp>
#include <pjh_cli/console.hpp>
#include <pjh_cli/console/history.hpp>
#include <pjh_cli/console/in_memory_history.hpp>
#include <pjh_cli/console/noop_history.hpp>
#include <pjh_result.hpp>
#include <sstream>
#include <string>
#include <string_view>

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

TEST_CASE("InteractiveConsole history not pushed for help")
{
    App app("test", "1.0", "History help");
    app.add_leaf("serve", "Server");

    std::stringstream input, output, error;
    auto hist = std::make_unique<InMemoryHistory>();
    auto *raw = hist.get();
    InteractiveConsole console(app, "> ", input, output, error, {}, {}, std::move(hist));

    auto r = console.process_line("help");
    CHECK(r.is_ok());
    CHECK(raw->size() == 0);
}

TEST_CASE("InteractiveConsole history not pushed for query")
{
    App app("test", "1.0", "History query");
    app.add_leaf("serve", "Server");

    std::stringstream input, output, error;
    auto hist = std::make_unique<InMemoryHistory>();
    auto *raw = hist.get();
    InteractiveConsole console(app, "> ", input, output, error, {}, {}, std::move(hist));

    auto r = console.process_line("?");
    CHECK(r.is_ok());
    CHECK(raw->size() == 0);
}

TEST_CASE("InteractiveConsole history pushed even on parse error")
{
    App app("test", "1.0", "History parse err");
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
