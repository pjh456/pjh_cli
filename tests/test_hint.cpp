#include <doctest/doctest.h>

#include <iostream>
#include <pjh_cli.hpp>
#include <string>
#include <string_view>

using namespace pjh::cli;

TEST_CASE("format_hint empty input on leaf")
{
    LeafCommand root("copy", "Copy");
    root.arg<std::string, 0>("src", "Source").required();
    root.arg<std::string, 1>("dst", "Destination").required();

    CHECK(HintBuilder::format(root, "") == "<src> <dst>");
}

TEST_CASE("format_hint empty input on branch")
{
    App app("test", "1.0", "Test");
    app.add_leaf("serve", "Serve");

    CHECK(HintBuilder::format(app, "").empty());
}

TEST_CASE("format_hint shows options and args")
{
    LeafCommand root("copy", "Copy");
    root.option<fixed_string("port")>("--port", 'p', "Port", 8080);
    root.option<fixed_string("verbose")>("--verbose", 'v', "Verbose").boolean();
    root.option<fixed_string("timeout")>("--timeout", 't', "Timeout")
        .integer()
        .required();
    root.arg<std::string, 0>("src", "Source").required();
    root.arg<std::string, 1>("dst", "Destination");

    auto hint = HintBuilder::format(root, "");
    CHECK(hint.find("[INT:port]") != std::string_view::npos);
    CHECK(hint.find("[BOOL:verbose]") != std::string_view::npos);
    CHECK(hint.find("INT:timeout") != std::string_view::npos);
    CHECK(hint.find("<src>") != std::string_view::npos);
    CHECK(hint.find("<dst>") != std::string_view::npos);
}

TEST_CASE("format_hint options always shown regardless of input")
{
    LeafCommand root("copy", "Copy");
    root.option<fixed_string("port")>("--port", 'p', "Port", 8080);
    root.option<fixed_string("verbose")>("--verbose", 'v', "Verbose").boolean();
    root.arg<std::string, 0>("src", "Source").required();

    // Options are always shown (order-independent); only positional
    // args are tracked for "remaining".
    auto hint = HintBuilder::format(root, "-v --port data.txt");
    CHECK(hint.find("[INT:port]") != std::string_view::npos);
    CHECK(hint.find("[BOOL:verbose]") != std::string_view::npos);
    CHECK(hint.find("<src>") == std::string_view::npos);  // consumed
}

TEST_CASE("format_hint descends into subcommand")
{
    App app("test", "1.0", "Test");
    auto &leaf = app.add_leaf("serve", "Serve");
    leaf.option<fixed_string("port")>("--port", 'p', "Port", 8080);
    leaf.arg<std::string, 0>("file", "File").required();

    auto hint = HintBuilder::format(app, "serve");
    CHECK(hint.find("[INT:port]") != std::string_view::npos);
    CHECK(hint.find("<file>") != std::string_view::npos);
}

TEST_CASE("format_hint descends with options before subcommand")
{
    App app("test", "1.0", "Test");
    app.option<fixed_string("verbose")>("--verbose", 'v', "Verbose").boolean();
    auto &leaf = app.add_leaf("serve", "Serve");
    leaf.option<fixed_string("port")>("--port", 'p', "Port", 8080);

    auto hint = HintBuilder::format(app, "-v serve");
    // Descend to serve, show serve's option
    CHECK(hint == "[INT:port]");
}

TEST_CASE("format_hint with consumed positional args")
{
    LeafCommand root("copy", "Copy");
    root.arg<std::string, 0>("src", "Source").required();
    root.arg<std::string, 1>("dst", "Destination").required();

    auto hint = HintBuilder::format(root, "fileA.txt");
    CHECK(hint == "<dst>");
}

TEST_CASE("format_hint with all positional args consumed")
{
    LeafCommand root("copy", "Copy");
    root.arg<std::string, 0>("src", "Source").required();
    root.arg<std::string, 1>("dst", "Destination").required();

    auto hint = HintBuilder::format(root, "fileA.txt fileB.txt");
    CHECK(hint.empty());
}

TEST_CASE("format_hint option_mode Required")
{
    LeafCommand root("copy", "Copy");
    root.option<fixed_string("port")>("--port", 'p', "Port", 8080);
    root.option<fixed_string("timeout")>("--timeout", 't', "Timeout")
        .integer()
        .required();

    auto hint = HintBuilder::format(root, "", HintConfig{HintOptionMode::Required});
    // port is optional → hidden, timeout is required → shown
    CHECK(hint.find("[INT:port]") == std::string_view::npos);
    CHECK(hint.find("INT:timeout") != std::string_view::npos);
}

TEST_CASE("format_hint option_mode None")
{
    LeafCommand root("copy", "Copy");
    root.option<fixed_string("port")>("--port", 'p', "Port", 8080);
    root.option<fixed_string("timeout")>("--timeout", 't', "Timeout")
        .integer()
        .required();
    root.arg<std::string, 0>("src", "Source").required();

    auto hint = HintBuilder::format(root, "", HintConfig{HintOptionMode::None});
    CHECK(hint.find("INT") == std::string_view::npos);
    CHECK(hint.find("BOOL") == std::string_view::npos);
    CHECK(hint == "<src>");
}

TEST_CASE("format_hint subcommand with consumed positional arg")
{
    App app("test", "1.0", "Test");
    auto &leaf = app.add_leaf("cmd", "Command");
    leaf.arg<std::string, 0>("file", "File").required();
    leaf.arg<std::string, 1>("extra", "Extra");

    auto hint = HintBuilder::format(app, "cmd data.txt");
    CHECK(hint == "<extra>");
}

TEST_CASE("format_hint multi-level subcommand")
{
    App app("test", "1.0", "Test");
    auto &mid = app.add_branch("mid", "Middle");
    auto &leaf = mid.add_leaf("leaf", "Leaf");
    leaf.arg<std::string, 0>("file", "File").required();

    auto hint = HintBuilder::format(app, "mid leaf");
    CHECK(hint == "<file>");
}
