#include <doctest/doctest.h>

#include <iostream>
#include <pjh_cli/app.hpp>
#include <pjh_cli/core/error.hpp>
#include <pjh_cli/core/fixed_string.hpp>
#include <pjh_result.hpp>
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

TEST_CASE("App::run empty help formatter falls back and skips the action")
{
    App app("test", "1.0", "Empty run formatter");
    app.option<fixed_string("port")>("--port", "Port").integer().required();
    int called = 0;
    app.action(
        [&called](ParseContext &) -> CliResult<void>
        {
            ++called;
            return CliResult<void>::Ok();
        });
    app.set_help_formatter([](const BaseCommand &) { return std::string{}; });
    std::ostringstream out, err;
    Argv argv{"test", "--help"};

    CHECK(app.run(argv.argc(), argv.argv(), out, err) == kExitSuccess);
    CHECK(called == 0);                           // action must not run
    CHECK(out.str().starts_with("Usage: test"));  // built-in help printed
    CHECK(err.str().empty());
}

TEST_CASE("App::run subcommand empty help formatter falls back and skips the action")
{
    App app("test", "1.0", "Empty subcommand formatter");
    int called = 0;
    app.add_leaf("serve", "Start server")
        .action(
            [&called](ParseContext &) -> CliResult<void>
            {
                ++called;
                return CliResult<void>::Ok();
            });
    app.set_help_formatter([](const BaseCommand &) { return std::string{}; });
    std::ostringstream out, err;
    Argv argv{"test", "serve", "--help"};

    CHECK(app.run(argv.argc(), argv.argv(), out, err) == kExitSuccess);
    CHECK(called == 0);                                 // action must not run
    CHECK(out.str().starts_with("Usage: test serve"));  // built-in help printed
    CHECK(err.str().empty());
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

TEST_CASE("App::run_quiet executes the action without writing to streams")
{
    App app("test", "1.0", "Quiet action");
    int called = 0;
    app.add_leaf("greet", "Greet")
        .action(
            [&called](ParseContext &) -> CliResult<void>
            {
                ++called;
                return CliResult<void>::Ok();
            });
    Argv argv{"test", "greet"};
    AppRunResult result;
    {
        IoCapture cap;
        result = app.run_quiet(argv.argc(), argv.argv());
        CHECK(cap.out.str().empty());
        CHECK(cap.err.str().empty());
    }

    CHECK(result.kind == AppRunResult::Kind::Success);
    CHECK(result.error.is_none());
    CHECK(result.exit_code() == kExitSuccess);
    CHECK(result.text.empty());
    CHECK(called == 1);
}

TEST_CASE("App::run_quiet returns help text without printing")
{
    App app("test", "1.0", "Quiet help");
    Argv argv{"test", "--help"};
    AppRunResult result;
    {
        IoCapture cap;
        result = app.run_quiet(argv.argc(), argv.argv());
        CHECK(cap.out.str().empty());
        CHECK(cap.err.str().empty());
    }

    CHECK(result.kind == AppRunResult::Kind::Help);
    CHECK(result.text.starts_with("Usage: test"));
    CHECK(result.error.is_none());
    CHECK(result.exit_code() == kExitSuccess);
}

TEST_CASE("App::run_quiet returns version text without printing")
{
    App app("test", "1.0", "Quiet version");
    Argv argv{"test", "--version"};
    AppRunResult result;
    {
        IoCapture cap;
        result = app.run_quiet(argv.argc(), argv.argv());
        CHECK(cap.out.str().empty());
        CHECK(cap.err.str().empty());
    }

    CHECK(result.kind == AppRunResult::Kind::Version);
    CHECK(result.text == "test version 1.0\n");
    CHECK(result.error.is_none());
    CHECK(result.exit_code() == kExitSuccess);
}

TEST_CASE("App::run_quiet reports a parse error")
{
    App app("test", "1.0", "Quiet parse error");
    Argv argv{"test", "--bogus"};
    AppRunResult result;
    {
        IoCapture cap;
        result = app.run_quiet(argv.argc(), argv.argv());
        CHECK(cap.out.str().empty());
        CHECK(cap.err.str().empty());
    }

    REQUIRE(result.error.is_some());
    CHECK(result.kind == AppRunResult::Kind::ParseError);
    CHECK(result.error.unwrap().kind() == ErrorKind::Parse);
    CHECK(result.error.unwrap().tag() == ErrorTag::UnknownOption);
    CHECK(result.exit_code() == kExitParseError);
}

TEST_CASE("App::run_quiet reports a runtime action error")
{
    App app("test", "1.0", "Quiet runtime error");
    app.action(
        [](ParseContext &) -> CliResult<void> { return CliFailure{CliError("boom")}; });
    Argv argv{"test"};

    auto result = app.run_quiet(argv.argc(), argv.argv());

    REQUIRE(result.error.is_some());
    CHECK(result.kind == AppRunResult::Kind::RuntimeError);
    CHECK(std::string(result.error.unwrap().what()) == "boom");
    CHECK(result.exit_code() == kExitRuntimeError);
}

TEST_CASE("App::run_quiet reports a parse-kind action error")
{
    App app("test", "1.0", "Quiet parse-kind error");
    app.action(
        [](ParseContext &) -> CliResult<void>
        { return CliFailure{ErrorFactory::parse_error("x", 0)}; });
    Argv argv{"test"};

    auto result = app.run_quiet(argv.argc(), argv.argv());

    REQUIRE(result.error.is_some());
    CHECK(result.kind == AppRunResult::Kind::ParseError);
    CHECK(result.exit_code() == kExitParseError);
}

TEST_CASE("App::run_quiet error composes with the Diagnostic protocol")
{
    App app("test", "1.0", "Quiet diagnostic");
    Argv argv{"test", "--bogus"};

    auto result = app.run_quiet(argv.argc(), argv.argv());

    REQUIRE(result.error.is_some());
    const CliError &error = result.error.unwrap();
    CHECK(error.diagnostic().kind() == ErrorTag::UnknownOption);
    CHECK(pjh::result::render(error.diagnostic()) == std::string(error.what()));
}

TEST_CASE("App::run and run_quiet agree on output and exit codes")
{
    {
        App app("test", "1.0", "Agree help");
        Argv argv{"test", "--help"};
        std::ostringstream out, err;

        int code = app.run(argv.argc(), argv.argv(), out, err);
        auto quiet = app.run_quiet(argv.argc(), argv.argv());

        CHECK(quiet.kind == AppRunResult::Kind::Help);
        CHECK(out.str() == quiet.text);
        CHECK(err.str().empty());
        CHECK(code == quiet.exit_code());
    }
    {
        App app("test", "1.0", "Agree version");
        Argv argv{"test", "--version"};
        std::ostringstream out, err;

        int code = app.run(argv.argc(), argv.argv(), out, err);
        auto quiet = app.run_quiet(argv.argc(), argv.argv());

        CHECK(quiet.kind == AppRunResult::Kind::Version);
        CHECK(out.str() == quiet.text);
        CHECK(err.str().empty());
        CHECK(code == quiet.exit_code());
    }
    {
        App app("test", "1.0", "Agree parse error");
        Argv argv{"test", "--bogus"};
        std::ostringstream out, err;

        int code = app.run(argv.argc(), argv.argv(), out, err);
        auto quiet = app.run_quiet(argv.argc(), argv.argv());

        REQUIRE(quiet.error.is_some());
        CHECK(out.str().empty());
        CHECK(err.str() == std::string(quiet.error.unwrap().what()) + "\n");
        CHECK(code == quiet.exit_code());
    }
    {
        App app("test", "1.0", "Agree success");
        int called = 0;
        app.action(
            [&called](ParseContext &) -> CliResult<void>
            {
                ++called;
                return CliResult<void>::Ok();
            });
        Argv argv{"test"};
        std::ostringstream out, err;

        int code = app.run(argv.argc(), argv.argv(), out, err);
        auto quiet = app.run_quiet(argv.argc(), argv.argv());

        CHECK(quiet.kind == AppRunResult::Kind::Success);
        CHECK(out.str() == quiet.text);
        CHECK(err.str().empty());
        CHECK(code == quiet.exit_code());
        CHECK(called == 2);
    }
    {
        App app("test", "1.0", "Agree runtime error");
        app.action(
            [](ParseContext &) -> CliResult<void>
            { return CliFailure{CliError("boom")}; });
        Argv argv{"test"};
        std::ostringstream out, err;

        int code = app.run(argv.argc(), argv.argv(), out, err);
        auto quiet = app.run_quiet(argv.argc(), argv.argv());

        REQUIRE(quiet.error.is_some());
        CHECK(out.str().empty());
        CHECK(err.str() == std::string(quiet.error.unwrap().what()) + "\n");
        CHECK(code == quiet.exit_code());
    }
}

TEST_CASE("App::run_fuzzy_quiet auto-corrects a typo")
{
    App app("test", "1.0", "Quiet fuzzy");
    int called = 0;
    app.add_leaf("install", "Install")
        .action(
            [&called](ParseContext &) -> CliResult<void>
            {
                ++called;
                return CliResult<void>::Ok();
            });
    Argv argv{"test", "instal"};

    auto result = app.run_fuzzy_quiet(argv.argc(), argv.argv());

    CHECK(result.kind == AppRunResult::Kind::Success);
    CHECK(result.error.is_none());
    CHECK(result.exit_code() == kExitSuccess);
    CHECK(called == 1);
}

TEST_CASE("App::run_fuzzy_quiet reports ambiguity as a parse error")
{
    App app("test", "1.0", "Quiet fuzzy ambiguous");
    app.add_leaf("start", "Start");
    app.add_leaf("stop", "Stop");
    Argv argv{"test", "st"};

    auto result = app.run_fuzzy_quiet(argv.argc(), argv.argv());

    REQUIRE(result.error.is_some());
    CHECK(result.kind == AppRunResult::Kind::ParseError);
    CHECK(result.exit_code() == kExitParseError);
    CHECK(
        std::string(result.error.unwrap().what()).find("ambiguous command 'st'") !=
        std::string::npos);
}

TEST_CASE("App::run_quiet with no tokens executes the root action")
{
    App app("test", "1.0", "Quiet root");
    int called = 0;
    app.action(
        [&called](ParseContext &) -> CliResult<void>
        {
            ++called;
            return CliResult<void>::Ok();
        });
    Argv argv{"test"};
    AppRunResult result;
    {
        IoCapture cap;
        result = app.run_quiet(argv.argc(), argv.argv());
        CHECK(cap.out.str().empty());
        CHECK(cap.err.str().empty());
    }

    CHECK(result.kind == AppRunResult::Kind::Success);
    CHECK(called == 1);
}

TEST_CASE("AppRunResult::exit_code maps NoCommand to 2")
{
    AppRunResult result;
    result.kind = AppRunResult::Kind::NoCommand;
    result.error =
        pjh::result::Option<CliError>::Some(ErrorFactory::no_command_matched());

    CHECK(result.exit_code() == kExitParseError);
}
