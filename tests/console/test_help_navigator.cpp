#include <doctest/doctest.h>

#include <iostream>
#include <pjh_cli/app.hpp>
#include <pjh_cli/console.hpp>
#include <pjh_cli/console/help_navigator.hpp>
#include <pjh_cli/format/info.hpp>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

using namespace pjh::cli;

// ── HelpNavigator standalone tests (no I/O) ──

TEST_CASE("HelpNavigator root help (tokens.size() == 1)")
{
    App app("test", "1.0", "Root help test");
    app.add_leaf("serve", "Start the server");

    std::vector<std::string> tokens{"help"};
    auto result = HelpNavigator::navigate(app, tokens);

    CHECK(result.kind == HelpNavigationKind::RootHelp);
    REQUIRE(result.resolved != nullptr);
    CHECK(result.resolved->name() == "test");
}

TEST_CASE("HelpNavigator navigate to subcommand")
{
    App app("test", "1.0", "Nav test");
    app.add_leaf("serve", "Start the server");

    std::vector<std::string> tokens{"help", "serve"};
    auto result = HelpNavigator::navigate(app, tokens);

    CHECK(result.kind == HelpNavigationKind::SubcommandHelp);
    REQUIRE(result.resolved != nullptr);
    CHECK(result.resolved->name() == "serve");
}

TEST_CASE("HelpNavigator navigate nested subcommands")
{
    App app("test", "1.0", "Nested");
    auto &container = app.add_branch("container", "Container");
    container.add_leaf("start", "Start");

    std::vector<std::string> tokens{"help", "container", "start"};
    auto result = HelpNavigator::navigate(app, tokens);

    CHECK(result.kind == HelpNavigationKind::SubcommandHelp);
    REQUIRE(result.resolved != nullptr);
    CHECK(result.resolved->name() == "start");
}

TEST_CASE("HelpNavigator leaf has no subcommands")
{
    App app("test", "1.0", "Leaf test");
    app.add_leaf("serve", "Start the server");

    std::vector<std::string> tokens{"help", "serve", "extra"};
    auto result = HelpNavigator::navigate(app, tokens);

    CHECK(result.kind == HelpNavigationKind::NonBranch);
    CHECK(result.failed_command_name == "serve");
}

TEST_CASE("HelpNavigator unknown subcommand")
{
    App app("test", "1.0", "Unknown");
    app.add_leaf("serve", "Start the server");

    std::vector<std::string> tokens{"help", "zzzzz"};
    auto result = HelpNavigator::navigate(app, tokens);

    CHECK(result.kind == HelpNavigationKind::UnknownCommand);
    CHECK(result.failed_token == "zzzzz");
    CHECK(result.suggestions.matches.empty());
}

TEST_CASE("HelpNavigator unknown with fuzzy suggestions")
{
    App app("test", "1.0", "Fuzzy help");
    app.add_leaf("server", "Server command");

    std::vector<std::string> tokens{"help", "servr"};
    auto result = HelpNavigator::navigate(app, tokens);

    CHECK(result.kind == HelpNavigationKind::UnknownCommand);
    CHECK(result.failed_token == "servr");
    REQUIRE(!result.suggestions.matches.empty());
    CHECK(result.suggestions.matches[0].name == "server");
}

// ── HelpNavigationOutput format tests ──

TEST_CASE("HelpNavigationOutput root help")
{
    App app("test", "1.0", "Root");
    app.add_leaf("serve", "Start");

    std::vector<std::string> tokens{"help"};
    auto output = HelpNavigationOutput::format(app, tokens);

    CHECK(output.starts_with("Usage: test <command>"));
    CHECK(output.find("Usage:") != std::string_view::npos);
    CHECK(output.find("serve") != std::string_view::npos);
}

TEST_CASE("HelpNavigationOutput subcommand help")
{
    App app("test", "1.0", "Sub");
    app.add_leaf("serve", "Start the server");

    std::vector<std::string> tokens{"help", "serve"};
    auto output = HelpNavigationOutput::format(app, tokens);

    CHECK(output.starts_with("Usage: test serve"));
    CHECK(output.find("Usage:") != std::string_view::npos);
    CHECK(output.find("serve") != std::string_view::npos);
}

TEST_CASE("HelpNavigationOutput nested subcommand shows full path")
{
    App app("test", "1.0", "Nested out");
    auto &container = app.add_branch("container", "Container");
    container.add_leaf("start", "Start");

    std::vector<std::string> tokens{"help", "container", "start"};
    auto output = HelpNavigationOutput::format(app, tokens);

    CHECK(output.starts_with("Usage: test container start"));
}

TEST_CASE("HelpNavigationOutput non-branch message")
{
    App app("test", "1.0", "NonBranch");
    app.add_leaf("serve", "Start");

    std::vector<std::string> tokens{"help", "serve", "extra"};
    auto output = HelpNavigationOutput::format(app, tokens);

    CHECK(output.find("has no subcommands") != std::string_view::npos);
    CHECK(output.find("serve") != std::string_view::npos);
}

TEST_CASE("HelpNavigationOutput unknown command")
{
    App app("test", "1.0", "Unknown out");
    app.add_leaf("serve", "Start");

    std::vector<std::string> tokens{"help", "zzzzz"};
    auto output = HelpNavigationOutput::format(app, tokens);

    CHECK(output.find("Unknown subcommand") != std::string_view::npos);
    CHECK(output.find("zzzzz") != std::string_view::npos);
}

TEST_CASE("HelpNavigationOutput unknown with fuzzy")
{
    App app("test", "1.0", "Fuzzy out");
    app.add_leaf("server", "Server command");

    std::vector<std::string> tokens{"help", "servr"};
    auto output = HelpNavigationOutput::format(app, tokens);

    CHECK(output.find("Unknown subcommand") != std::string_view::npos);
    CHECK(output.find("Did you mean:") != std::string_view::npos);
    CHECK(output.find("server") != std::string_view::npos);
}

// ── Custom formatter tests ──

TEST_CASE("InteractiveConsole with custom help formatter")
{
    App app("test", "1.0", "Console custom help");
    app.add_leaf("serve", "Start");

    std::stringstream input, output, error;
    int called = 0;
    auto custom = [&](const HelpNavigationResult &r) -> std::string
    {
        ++called;
        return "[custom help]";
    };

    InteractiveConsole console(app, "> ", input, output, error,
        std::function<std::string(const QueryResult &)>{},
        custom);
    auto r = console.process_line("help");
    CHECK(r.is_ok());
    CHECK(called == 1);
    CHECK(output.str().find("[custom help]") != std::string_view::npos);
}

TEST_CASE("InteractiveConsole with custom help formatter for unknown command")
{
    App app("test", "1.0", "Console unknown custom");
    app.add_leaf("serve", "Start");

    std::stringstream input, output, error;
    int called = 0;
    auto custom = [&](const HelpNavigationResult &r) -> std::string
    {
        ++called;
        CHECK(r.kind == HelpNavigationKind::UnknownCommand);
        return "[unknown:" + r.failed_token + "]";
    };

    InteractiveConsole console(app, "> ", input, output, error,
        std::function<std::string(const QueryResult &)>{},
        custom);
    auto r = console.process_line("help nope");
    CHECK(r.is_ok());
    CHECK(called == 1);
    CHECK(output.str().find("[unknown:nope]") != std::string_view::npos);
}