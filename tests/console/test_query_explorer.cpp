#include <doctest/doctest.h>

#include <pjh_cli/app.hpp>
#include <pjh_cli/console.hpp>
#include <pjh_cli/console/query_explorer.hpp>
#include <pjh_cli/console/query_output.hpp>
#include <sstream>
#include <string>
#include <string_view>

using namespace pjh::cli;

TEST_CASE("QueryExplorer empty query returns listing")
{
    App app("test", "1.0", "Test");
    app.add_leaf("foo", "Foo command");
    app.add_leaf("bar", "Bar command");

    auto result = QueryExplorer::explore(app, "");
    CHECK(result.kind == QueryKind::Listing);
    REQUIRE(result.names.size() == 2);
    CHECK(result.names[0] == "foo");
    CHECK(result.names[1] == "bar");
}

TEST_CASE("QueryExplorer empty query with no subcommands")
{
    App app("test", "1.0", "Test");

    auto result = QueryExplorer::explore(app, "");
    CHECK(result.kind == QueryKind::Listing);
    CHECK(result.names.empty());
}

TEST_CASE("QueryExplorer substring match by name")
{
    App app("test", "1.0", "Test");
    app.add_leaf("server", "Server command");
    app.add_leaf("config", "Config command");

    auto result = QueryExplorer::explore(app, "serv");
    CHECK(result.kind == QueryKind::Matched);
    REQUIRE(result.names.size() == 1);
    CHECK(result.names[0] == "server");
}

TEST_CASE("QueryExplorer substring match by alias")
{
    App app("test", "1.0", "Test");
    auto &srv = app.add_leaf("server", "Server command");
    srv.alias("s");

    auto result = QueryExplorer::explore(app, "s");
    CHECK(result.kind == QueryKind::Matched);
    REQUIRE(result.names.size() == 1);
    CHECK(result.names[0] == "server");
}

TEST_CASE("QueryExplorer substring match multiple")
{
    App app("test", "1.0", "Test");
    app.add_leaf("server", "Server");
    app.add_leaf("secure", "Secure");
    app.add_leaf("config", "Config");

    auto result = QueryExplorer::explore(app, "se");
    CHECK(result.kind == QueryKind::Matched);
    REQUIRE(result.names.size() == 2);
    CHECK(result.names[0] == "server");
    CHECK(result.names[1] == "secure");
}

TEST_CASE("QueryExplorer fuzzy fallback")
{
    App app("test", "1.0", "Test");
    app.add_leaf("server", "Server command");

    auto result = QueryExplorer::explore(app, "servr");
    CHECK(result.kind == QueryKind::Fuzzy);
    REQUIRE(result.suggestions.matches.size() == 1);
    CHECK(result.suggestions.matches[0].name == "server");
}

TEST_CASE("QueryExplorer no match")
{
    App app("test", "1.0", "Test");
    app.add_leaf("server", "Server command");

    auto result = QueryExplorer::explore(app, "zzzzz");
    CHECK(result.kind == QueryKind::NoMatch);
    CHECK(!result.usage_line.empty());
}

TEST_CASE("QueryExplorer hidden subcommands excluded")
{
    App app("test", "1.0", "Test");
    app.add_leaf("visible", "Visible");
    app.add_leaf("hidden", "Hidden").set_visibility(Visibility::Hidden);

    auto result = QueryExplorer::explore(app, "");
    CHECK(result.kind == QueryKind::Listing);
    REQUIRE(result.names.size() == 1);
    CHECK(result.names[0] == "visible");
}

TEST_CASE("QueryExplorer disabled subcommands excluded")
{
    App app("test", "1.0", "Test");
    app.add_leaf("active", "Active");
    app.add_leaf("inactive", "Inactive").enabled([] { return false; });

    auto result = QueryExplorer::explore(app, "");
    CHECK(result.kind == QueryKind::Listing);
    REQUIRE(result.names.size() == 1);
    CHECK(result.names[0] == "active");
}

TEST_CASE("QueryOutput format listing")
{
    App app("test", "1.0", "Test");
    app.add_leaf("foo", "Foo");
    app.add_leaf("bar", "Bar");

    auto output = QueryOutput::format(app, "");
    CHECK(output.find("Subcommands:") != std::string_view::npos);
    CHECK(output.find("foo") != std::string_view::npos);
    CHECK(output.find("bar") != std::string_view::npos);
}

TEST_CASE("QueryOutput format matched")
{
    App app("test", "1.0", "Test");
    app.add_leaf("server", "Server");

    auto output = QueryOutput::format(app, "serv");
    CHECK(output.find("Matching subcommands:") != std::string_view::npos);
    CHECK(output.find("server") != std::string_view::npos);
}

TEST_CASE("QueryOutput format fuzzy")
{
    App app("test", "1.0", "Test");
    app.add_leaf("server", "Server");

    auto output = QueryOutput::format(app, "servr");
    CHECK(output.find("Did you mean:") != std::string_view::npos);
    CHECK(output.find("server") != std::string_view::npos);
}

TEST_CASE("QueryOutput format no match")
{
    App app("test", "1.0", "Test");
    app.add_leaf("server", "Server");

    auto output = QueryOutput::format(app, "zzzzz");
    CHECK(output.find("No matches.") != std::string_view::npos);
}

TEST_CASE("QueryOutput with custom formatter")
{
    App app("test", "1.0", "Test");
    app.add_leaf("server", "Server");

    auto custom = [](const QueryResult &r)
    {
        CHECK(r.kind == QueryKind::Matched);
        return "custom:" + r.names[0];
    };

    auto output = QueryOutput::format(app, "serv", custom);
    CHECK(output == "custom:server");
}

TEST_CASE("InteractiveConsole with custom query formatter")
{
    App app("test", "1.0", "Test");
    app.add_leaf("server", "Server");

    std::stringstream input, output, error;
    int called = 0;
    auto custom = [&](const QueryResult &r) -> std::string
    {
        ++called;
        return "[custom] " + r.names[0];
    };

    InteractiveConsole console(app, "> ", input, output, error, custom);
    auto r = console.process_line("?serv");
    CHECK(r.is_ok());
    CHECK(called == 1);
    CHECK(output.str().find("[custom] server") != std::string_view::npos);
}