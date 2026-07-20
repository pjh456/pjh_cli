#include <doctest/doctest.h>

#include <initializer_list>
#include <pjh_cli/matcher.hpp>
#include <string>
#include <string_view>
#include <vector>

#include "pjh_cli/app.hpp"
#include "pjh_cli/command/base_command.hpp"
#include "pjh_cli/command/leaf_command.hpp"
#include "pjh_cli/fixed_string.hpp"

using namespace pjh::cli;

namespace
{
    struct Argv
    {
        std::vector<std::string> storage;
        std::vector<char *> ptrs;

        Argv(std::initializer_list<std::string> list) : storage(list)
        {
            for (auto &s : storage) ptrs.push_back(s.data());
        }

        int argc() const { return static_cast<int>(ptrs.size()); }
        char **argv() { return ptrs.data(); }
    };
}

TEST_CASE("edit_distance")
{
    CHECK(edit_distance("", "") == 0);
    CHECK(edit_distance("abc", "abc") == 0);
    CHECK(edit_distance("abc", "ab") == 1);
    CHECK(edit_distance("abc", "abcd") == 1);
    CHECK(edit_distance("abc", "abd") == 1);
    CHECK(edit_distance("abc", "xyz") == 3);
    CHECK(edit_distance("serve", "server") == 1);
    CHECK(edit_distance("start", "stop") == 3);
}

TEST_CASE("fuzzy find exact match")
{
    App app("test", "1.0", "Fuzzy test");
    app.add_leaf("server", "Start server");
    app.add_leaf("config", "Configuration");
    app.add_leaf("deploy", "Deploy app");
    app.add_leaf("help", "Show help");

    auto matches = fuzzy_find_subcommands(app, "server", 2);
    CHECK(matches.size() >= 1);
    CHECK(matches[0].distance == 0);
    CHECK(matches[0].command->name() == "server");
}

TEST_CASE("fuzzy find fuzzy match")
{
    App app("test", "1.0", "Fuzzy test");
    app.add_leaf("server", "Start server");
    app.add_leaf("config", "Configuration");
    app.add_leaf("deploy", "Deploy app");
    app.add_leaf("help", "Show help");

    auto matches = fuzzy_find_subcommands(app, "serv", 2);
    CHECK(!matches.empty());
    CHECK(matches[0].command->name() == "server");
    CHECK(matches[0].distance <= 2);
}

TEST_CASE("fuzzy find no match beyond threshold")
{
    App app("test", "1.0", "Fuzzy test");
    app.add_leaf("server", "Start server");
    app.add_leaf("config", "Configuration");
    app.add_leaf("deploy", "Deploy app");
    app.add_leaf("help", "Show help");

    auto matches = fuzzy_find_subcommands(app, "zzzzz", 2);
    CHECK(matches.empty());
}

TEST_CASE("fuzzy find respects visibility")
{
    App app("test", "1.0", "Visibility test");
    app.add_leaf("visible", "Visible");
    auto &hidden = app.add_leaf("hidden", "Hidden");
    hidden.set_visibility(Visibility::Hidden);

    CHECK(!fuzzy_find_subcommands(app, "visible", 2).empty());
    CHECK(fuzzy_find_subcommands(app, "hidden", 2).empty());
}

TEST_CASE("list_subcommands")
{
    App app("test", "1.0", "List test");
    app.add_leaf("foo", "Foo");
    app.add_leaf("bar", "Bar");
    app.add_leaf("baz", "Baz");

    auto names = list_subcommands(app);
    CHECK(names.size() == 3);
}

TEST_CASE("complete subcommand prefix")
{
    App app("test", "1.0", "Complete test");
    app.add_leaf("server", "Server");
    app.add_leaf("serve", "Serve");

    auto candidates = complete(app, "ser");
    CHECK(!candidates.empty());
    bool has_server = false, has_serve = false;
    for (const auto &c : candidates)
    {
        if (c == "server")
            has_server = true;
        if (c == "serve")
            has_serve = true;
    }
    CHECK(has_server);
    CHECK(has_serve);

    CHECK(complete(app, "x").empty());
}

TEST_CASE("complete long option prefix")
{
    App app("test", "1.0", "Option complete");
    app.option<fixed_string("verbose")>("--verbose", 'v', "Verbose").boolean();
    app.option<fixed_string("port")>("--port", 'p', "Port").integer();

    auto candidates = complete(app, "--ver");
    CHECK(!candidates.empty());
    CHECK(candidates[0] == "--verbose");
}

TEST_CASE("complete short option prefix")
{
    App app("test", "1.0", "Option complete");
    app.option<fixed_string("verbose")>("--verbose", 'v', "Verbose").boolean();
    app.option<fixed_string("port")>("--port", 'p', "Port").integer();

    auto candidates = complete(app, "-");
    bool has_v = false, has_p = false;
    for (const auto &c : candidates)
    {
        if (c == "-v")
            has_v = true;
        if (c == "-p")
            has_p = true;
    }
    CHECK(has_v);
    CHECK(has_p);
}

TEST_CASE("format_usage")
{
    LeafCommand app("test", "Test app");
    app.option<fixed_string("port")>("--port", 'p', "Port", 8080);
    app.option<fixed_string("verbose")>("--verbose", 'v', "Verbose").boolean();
    app.arg<std::string, 0>("source", "Source file").required();
    app.arg<std::string, 1>("dest", "Destination");

    auto usage = format_usage(app, "test");
    CHECK(usage.find("Usage:") != std::string_view::npos);
    CHECK(usage.find("--port") != std::string_view::npos);
    CHECK(usage.find("--verbose") != std::string_view::npos);
    CHECK(usage.find("<source>") != std::string_view::npos);
    CHECK(usage.find("[dest]") != std::string_view::npos);
}

TEST_CASE("format_help with args")
{
    LeafCommand app("test", "Test application");
    app.option<fixed_string("port")>("--port", 'p', "Port number", 8080);
    app.option<fixed_string("verbose")>("--verbose", 'v', "Enable verbose").boolean();
    app.arg<std::string, 0>("file", "Input file").required();

    auto help = format_help(app, "test");
    CHECK(help.find("Usage:") != std::string_view::npos);
    CHECK(help.find("Test application") != std::string_view::npos);
    CHECK(help.find("Options:") != std::string_view::npos);
    CHECK(help.find("Arguments:") != std::string_view::npos);
    CHECK(help.find("<file>") != std::string_view::npos);
}

TEST_CASE("format_help with subcommands")
{
    App app("test", "1.0", "App with subcommands");
    app.add_leaf("serve", "Start the server");

    auto help = format_help(app, "test");
    CHECK(help.find("Usage:") != std::string_view::npos);
    CHECK(help.find("Subcommands:") != std::string_view::npos);
    CHECK(help.find("serve") != std::string_view::npos);
}

TEST_CASE("parse_fuzzy exact match")
{
    App app("test", "1.0", "Fuzzy parse");
    auto &serve = app.add_leaf("server", "Server");
    serve.option<fixed_string("port")>("--port", 'p', "Port", 8080);

    Argv argv{"test", "server", "--port", "3000"};
    auto r = app.parse_fuzzy(argv.argc(), argv.argv());
    CHECK(r.is_ok());
    auto port = r.unwrap().get<int, fixed_string("port")>();
    CHECK(port == 3000);
}

TEST_CASE("parse_fuzzy typo tolerant")
{
    App app("test", "1.0", "Fuzzy parse typo");
    app.add_leaf("server", "Server");
    app.add_leaf("config", "Config");

    Argv argv{"test", "servr"};
    auto r = app.parse_fuzzy(argv.argc(), argv.argv());
    CHECK(r.is_ok());
    CHECK(r.unwrap().matched_path() == "server");
}

TEST_CASE("parse_fuzzy ambiguous")
{
    App app("test", "1.0", "Ambiguous");
    app.add_leaf("start", "Start");
    app.add_leaf("stop", "Stop");

    Argv argv{"test", "st"};
    auto r = app.parse_fuzzy(argv.argc(), argv.argv());
    CHECK(r.is_ok());
}

TEST_CASE("parse_fuzzy no match")
{
    App app("test", "1.0", "No fuzzy match");
    app.add_leaf("server", "Server");

    Argv argv{"test", "zzzzz"};
    auto r = app.parse_fuzzy(argv.argc(), argv.argv());
    CHECK(r.is_ok());
}
