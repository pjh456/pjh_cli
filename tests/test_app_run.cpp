#include <doctest/doctest.h>

#include <iostream>
#include <pjh_cli/app.hpp>
#include <pjh_cli/core/error.hpp>
#include <pjh_cli/core/fixed_string.hpp>
#include <sstream>
#include <string>

#include "parser/test_helpers.hpp"

using namespace pjh::cli;

namespace
{
    /// @brief Temporarily redirects std::cout/std::cerr for default-overload
    ///        tests.
    struct IoCapture
    {
        std::ostringstream out;
        std::ostringstream err;
        std::streambuf *old_out = std::cout.rdbuf(out.rdbuf());
        std::streambuf *old_err = std::cerr.rdbuf(err.rdbuf());
        ~IoCapture()
        {
            std::cout.rdbuf(old_out);
            std::cerr.rdbuf(old_err);
        }
    };
}  // namespace

TEST_CASE("App::run executes the matched action and returns success")
{
    App app("test", "1.0", "Run action");
    int called = 0;
    app.add_leaf("greet", "Greet")
        .action(
            [&called](ParseContext &) -> CliResult<void>
            {
                ++called;
                return CliResult<void>::Ok();
            });
    std::ostringstream out, err;
    Argv argv{"test", "greet"};

    CHECK(app.run(argv.argc(), argv.argv(), out, err) == kExitSuccess);
    CHECK(called == 1);
    CHECK(out.str().empty());
    CHECK(err.str().empty());
}

TEST_CASE("App::run prints help to output and returns success")
{
    App app("test", "1.0", "Run help");
    std::ostringstream out, err;
    Argv argv{"test", "--help"};

    CHECK(app.run(argv.argc(), argv.argv(), out, err) == kExitSuccess);
    CHECK(out.str().starts_with("Usage: test"));
    CHECK(err.str().empty());
}

TEST_CASE("App::run prints version to output and returns success")
{
    App app("test", "1.0", "Run version");
    std::ostringstream out, err;
    Argv argv{"test", "--version"};

    CHECK(app.run(argv.argc(), argv.argv(), out, err) == kExitSuccess);
    CHECK(out.str() == "test version 1.0\n");
    CHECK(err.str().empty());
}

TEST_CASE("App::run maps parse errors to exit 2 on stderr")
{
    App app("test", "1.0", "Run parse error");
    std::ostringstream out, err;
    Argv argv{"test", "--bogus"};

    CHECK(app.run(argv.argc(), argv.argv(), out, err) == kExitParseError);
    CHECK(out.str().empty());
    CHECK(err.str() == "Parse Error: unknown option: '--bogus'\n");
}

TEST_CASE("App::run maps runtime action errors to exit 1 without a prefix")
{
    App app("test", "1.0", "Run runtime error");
    app.action(
        [](ParseContext &) -> CliResult<void> { return CliFailure{CliError("boom")}; });
    std::ostringstream out, err;
    Argv argv{"test"};

    CHECK(app.run(argv.argc(), argv.argv(), out, err) == kExitRuntimeError);
    CHECK(out.str().empty());
    CHECK(err.str() == "boom\n");  // ErrorKind::Runtime: no "Parse Error: "
}

TEST_CASE("App::run maps parse-kind action errors to exit 2")
{
    App app("test", "1.0", "Run parse-kind error");
    app.action(
        [](ParseContext &) -> CliResult<void>
        { return CliFailure{ErrorFactory::parse_error("x", 0)}; });
    std::ostringstream out, err;
    Argv argv{"test"};

    CHECK(app.run(argv.argc(), argv.argv(), out, err) == kExitParseError);
    CHECK(err.str().starts_with("Parse Error: "));
}

TEST_CASE("App::run honours the injected help formatter")
{
    App app("test", "1.0", "Run formatter");
    app.set_help_formatter([](const BaseCommand &) { return std::string("CUSTOM"); });
    std::ostringstream out, err;
    Argv argv{"test", "--help"};

    CHECK(app.run(argv.argc(), argv.argv(), out, err) == kExitSuccess);
    CHECK(out.str() == "CUSTOM");
}

TEST_CASE("App::run does not execute the action on a help request")
{
    App app("test", "1.0", "Run no action");
    int called = 0;
    app.action(
        [&called](ParseContext &) -> CliResult<void>
        {
            ++called;
            return CliResult<void>::Ok();
        });
    std::ostringstream out, err;
    Argv argv{"test", "-h"};

    CHECK(app.run(argv.argc(), argv.argv(), out, err) == kExitSuccess);
    CHECK(called == 0);
}

TEST_CASE("App::run_fuzzy auto-corrects a typo")
{
    App app("test", "1.0", "Run fuzzy");
    int called = 0;
    app.add_leaf("install", "Install")
        .action(
            [&called](ParseContext &) -> CliResult<void>
            {
                ++called;
                return CliResult<void>::Ok();
            });
    std::ostringstream out, err;
    Argv argv{"test", "instal"};

    CHECK(app.run_fuzzy(argv.argc(), argv.argv(), out, err) == kExitSuccess);
    CHECK(called == 1);
}

TEST_CASE("App::run_fuzzy reports ambiguity as exit 2")
{
    App app("test", "1.0", "Run fuzzy ambiguous");
    app.add_leaf("start", "Start");
    app.add_leaf("stop", "Stop");
    std::ostringstream out, err;
    Argv argv{"test", "st"};

    CHECK(app.run_fuzzy(argv.argc(), argv.argv(), out, err) == kExitParseError);
    CHECK(err.str().find("ambiguous command 'st'") != std::string::npos);
}

TEST_CASE("App::run default overload writes to std::cout and std::cerr")
{
    App app("test", "1.0", "Run default streams");
    Argv help{"test", "--help"};
    {
        IoCapture cap;
        CHECK(app.run(help.argc(), help.argv()) == kExitSuccess);
        CHECK(cap.out.str().starts_with("Usage: test"));
        CHECK(cap.err.str().empty());
    }
    Argv bad{"test", "--bogus"};
    {
        IoCapture cap;
        CHECK(app.run(bad.argc(), bad.argv()) == kExitParseError);
        CHECK(cap.out.str().empty());
        CHECK(cap.err.str().find("unknown option") != std::string::npos);
    }
}

TEST_CASE("App::run with no tokens executes the root action")
{
    App app("test", "1.0", "Run root");
    int called = 0;
    app.action(
        [&called](ParseContext &) -> CliResult<void>
        {
            ++called;
            return CliResult<void>::Ok();
        });
    std::ostringstream out, err;
    Argv argv{"test"};

    CHECK(app.run(argv.argc(), argv.argv(), out, err) == kExitSuccess);
    CHECK(called == 1);
    CHECK(out.str().empty());
}
