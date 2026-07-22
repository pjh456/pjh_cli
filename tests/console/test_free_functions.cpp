#include <doctest/doctest.h>

#include <string>
#include <string_view>
#include <vector>

#include <pjh_cli/app.hpp>
#include <pjh_cli/fixed_string.hpp>
#include <pjh_cli/help_formatter.hpp>
#include <pjh_cli/matcher.hpp>
#include <pjh_cli/parse_context.hpp>
#include <pjh_cli/parser.hpp>
#include <pjh_cli/type.hpp>

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
    app.option<fixed_string("port")>("--port", 'p', "Port").integer();

    std::vector<std::string_view> args{"--port", "9090"};
    auto r = Parser::parse_command(app, args);
    CHECK(r.is_ok());
    auto val = r.unwrap().get<int, fixed_string("port")>();
    CHECK(val == 9090);
}

TEST_CASE("parse_command fuzzy free function")
{
    App app("test", "1.0", "Free function fuzzy");
    app.add_leaf("server", "Server");
    app.add_leaf("config", "Config");

    std::vector<std::string_view> args{"servr"};
    auto r = Parser::parse_command(app, args, 3);
    CHECK(r.is_ok());
    CHECK(r.unwrap().matched_path() == "server");
}

TEST_CASE("const execute")
{
    App app("test", "1.0", "Const execute");
    int called = 0;
    app.action(
        [&called](ParseContext &) -> CliResult<void>
        {
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
    auto help = HelpFormatter::format_help(app, "test");
    CHECK(help.find("A test application") != std::string_view::npos);
    CHECK(help.find("Usage:") != std::string_view::npos);
    CHECK(help.find("test") != std::string_view::npos);
}

TEST_CASE("format usage includes program name")
{
    App app("myapp", "1.0", "My app");
    auto usage = HelpFormatter::format_usage(app, "myapp");
    CHECK(usage.find("Usage:") != std::string_view::npos);
    CHECK(usage.find("myapp") != std::string_view::npos);
}
