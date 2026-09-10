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
    // args are tracked for "remaining".  data.txt is --port's value, so it
    // is not a positional (hint does not convert/validate values).
    auto hint = HintBuilder::format(root, "-v --port data.txt");
    CHECK(hint.find("[INT:port]") != std::string_view::npos);
    CHECK(hint.find("[BOOL:verbose]") != std::string_view::npos);
    CHECK(hint.find("<src>") != std::string_view::npos);
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
    // Descend to serve; current options first, then inherited ancestors.
    CHECK(hint == "[INT:port] [BOOL:verbose]");
}

TEST_CASE("format_hint includes ancestor options after descent")
{
    App app("test", "1.0", "Test");
    app.option<fixed_string("verbose")>("--verbose", 'v', "Verbose").boolean();
    auto &leaf = app.add_leaf("serve", "Serve");
    leaf.option<fixed_string("port")>("--port", 'p', "Port", 8080);

    auto hint = HintBuilder::format(app, "serve");
    CHECK(hint.find("[BOOL:verbose]") != std::string_view::npos);
    CHECK(hint.find("[INT:port]") != std::string_view::npos);
}

TEST_CASE("format_hint nearest declaration wins")
{
    App app("test", "1.0", "Test");
    app.option<fixed_string("opt")>("--opt", "Root").integer();
    auto &son = app.add_leaf("son", "Son");
    son.option<fixed_string("opt")>("--opt", "Leaf").integer();

    CHECK(HintBuilder::format(app, "son") == "[INT:opt]");
}

TEST_CASE("format_hint inherited required option in Required mode")
{
    App app("test", "1.0", "Test");
    app.option<fixed_string("token")>("--token", 't', "Token").integer().required();
    auto &leaf = app.add_leaf("serve", "Serve");
    leaf.option<fixed_string("port")>("--port", 'p', "Port").integer();

    auto hint = HintBuilder::format(app, "serve", HintConfig{HintOptionMode::Required});
    CHECK(hint.find("INT:token") != std::string_view::npos);
    CHECK(hint.find("INT:port") == std::string_view::npos);
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

TEST_CASE("format_hint shows FLOAT STR PATH labels")
{
    LeafCommand root("copy", "Copy");
    root.option<fixed_string("rate")>("--rate", 'r', "Rate").floating();
    root.option<fixed_string("name")>("--name", 'n', "Name").str();
    root.option<fixed_string("out")>("--out", 'o', "Out").path();

    auto hint = HintBuilder::format(root, "");
    CHECK(hint.find("FLOAT:rate") != std::string_view::npos);
    CHECK(hint.find("STR:name") != std::string_view::npos);
    CHECK(hint.find("PATH:out") != std::string_view::npos);
}

TEST_CASE("format_hint counting option stays INT")
{
    LeafCommand root("copy", "Copy");
    root.option<fixed_string("verbose")>("--verbose", 'v', "Verbose").count();

    CHECK(HintBuilder::format(root, "") == "[INT:verbose]");
}

// ── option-value-aware positional counting ──

TEST_CASE("format_hint skips long option separate value")
{
    LeafCommand root("copy", "Copy");
    root.option<fixed_string("port")>("--port", 'p', "Port", 8080);
    root.arg<std::string, 0>("src", "Source").required();
    root.arg<std::string, 1>("dst", "Destination").required();

    auto hint =
        HintBuilder::format(root, "--port 8080", HintConfig{HintOptionMode::None});
    CHECK(hint == "<src> <dst>");  // HEAD: "<dst>"
}

TEST_CASE("format_hint skips short option separate value")
{
    LeafCommand root("copy", "Copy");
    root.option<fixed_string("port")>("--port", 'p', "Port", 8080);
    root.arg<std::string, 0>("src", "Source").required();
    root.arg<std::string, 1>("dst", "Destination").required();

    auto hint = HintBuilder::format(root, "-p 8080", HintConfig{HintOptionMode::None});
    CHECK(hint == "<src> <dst>");  // HEAD: "<dst>"
}

TEST_CASE("format_hint skips grouped short option value")
{
    LeafCommand root("copy", "Copy");
    root.option<fixed_string("verbose")>("--verbose", 'v', "Verbose").boolean();
    root.option<fixed_string("port")>("--port", 'p', "Port", 8080);
    root.arg<std::string, 0>("src", "Source").required();
    root.arg<std::string, 1>("dst", "Destination").required();

    auto hint = HintBuilder::format(root, "-vp 8080", HintConfig{HintOptionMode::None});
    CHECK(hint == "<src> <dst>");  // HEAD: "<dst>"
}

TEST_CASE("format_hint non-repeatable option value before positional")
{
    LeafCommand root("copy", "Copy");
    root.option<fixed_string("include")>("--include", 'I', "Include").str();
    root.arg<std::string, 0>("src", "Source").required();
    root.arg<std::string, 1>("dst", "Destination").required();

    auto hint =
        HintBuilder::format(root, "--include a src", HintConfig{HintOptionMode::None});
    CHECK(hint == "<dst>");  // 'a' is the value, 'src' is positional 0; HEAD: ""
}

TEST_CASE("format_hint skips inline equals and compact short values")
{
    LeafCommand root("copy", "Copy");
    root.option<fixed_string("port")>("--port", 'p', "Port", 8080);
    root.arg<std::string, 0>("src", "Source").required();
    root.arg<std::string, 1>("dst", "Destination").required();

    CHECK(
        HintBuilder::format(root, "--port=8080", HintConfig{HintOptionMode::None}) ==
        "<src> <dst>");  // already passes at HEAD; pin
    CHECK(
        HintBuilder::format(root, "-p8080", HintConfig{HintOptionMode::None}) ==
        "<src> <dst>");  // already passes at HEAD; pin
}

TEST_CASE("format_hint skips repeatable option greedy values")
{
    LeafCommand root("copy", "Copy");
    root.option<fixed_string("include")>("--include", 'I', "Include").path().repeatable();
    root.arg<std::string, 0>("src", "Source").required();
    root.arg<std::string, 1>("dst", "Destination").required();

    auto hint =
        HintBuilder::format(root, "--include a b c", HintConfig{HintOptionMode::None});
    CHECK(hint == "<src> <dst>");  // HEAD: "" (a,b,c counted)
}

TEST_CASE("format_hint repeatable stops at option flag")
{
    LeafCommand root("copy", "Copy");
    root.option<fixed_string("include")>("--include", 'I', "Include").path().repeatable();
    root.option<fixed_string("verbose")>("--verbose", 'v', "Verbose").boolean();
    root.arg<std::string, 0>("src", "Source").required();
    root.arg<std::string, 1>("dst", "Destination").required();

    auto hint = HintBuilder::format(
        root, "--include a --verbose b", HintConfig{HintOptionMode::None});
    CHECK(hint == "<dst>");  // a is a value, b is positional 0; HEAD: ""
}

TEST_CASE("format_hint compact short repeatable consumes following tokens")
{
    LeafCommand root("copy", "Copy");
    root.option<fixed_string("files")>("--files", 'f', "Files").path().repeatable();
    root.arg<std::string, 0>("src", "Source").required();
    root.arg<std::string, 1>("dst", "Destination").required();

    auto hint = HintBuilder::format(root, "-fa b c", HintConfig{HintOptionMode::None});
    CHECK(hint == "<src> <dst>");  // HEAD: ""
}

TEST_CASE("format_hint long equals repeatable does not consume following tokens")
{
    LeafCommand root("copy", "Copy");
    root.option<fixed_string("include")>("--include", 'I', "Include").path().repeatable();
    root.arg<std::string, 0>("src", "Source").required();
    root.arg<std::string, 1>("dst", "Destination").required();

    auto hint =
        HintBuilder::format(root, "--include=a b", HintConfig{HintOptionMode::None});
    CHECK(hint == "<dst>");  // 'a' attached, 'b' positional 0 (parser parity); HEAD: same
}

TEST_CASE("format_hint repeatable stops at subcommand name")
{
    App app("test", "1.0", "Repeatable subcommand");
    app.option<fixed_string("include")>("--include", 'I', "Include").path().repeatable();
    auto &leaf = app.add_leaf("serve", "Serve");
    leaf.arg<std::string, 0>("file", "File").required();
    leaf.arg<std::string, 1>("extra", "Extra");

    auto hint =
        HintBuilder::format(app, "--include a serve", HintConfig{HintOptionMode::None});
    CHECK(hint == "<file> <extra>");  // HEAD: "" (break before descent)
}

TEST_CASE("format_hint honors double-dash barrier")
{
    LeafCommand root("copy", "Copy");
    root.option<fixed_string("port")>("--port", 'p', "Port", 8080);
    root.arg<std::string, 0>("src", "Source").required();
    root.arg<std::string, 1>("dst", "Destination").required();

    auto hint = HintBuilder::format(root, "-- --port", HintConfig{HintOptionMode::None});
    CHECK(hint == "<dst>");  // --port is positional after --; HEAD: "<src> <dst>"
}

TEST_CASE("format_hint counts negative number as positional")
{
    LeafCommand root("copy", "Copy");
    root.arg<std::string, 0>("src", "Source").required();
    root.arg<std::string, 1>("dst", "Destination").required();

    auto hint = HintBuilder::format(root, "-5", HintConfig{HintOptionMode::None});
    CHECK(hint == "<dst>");  // HEAD: "<src> <dst>"
}

TEST_CASE("format_hint consumes negative number as option value")
{
    LeafCommand root("copy", "Copy");
    root.option<fixed_string("offset")>("--offset", 'o', "Offset").integer();
    root.arg<std::string, 0>("src", "Source").required();
    root.arg<std::string, 1>("dst", "Destination").required();

    auto hint = HintBuilder::format(root, "-o -5", HintConfig{HintOptionMode::None});
    CHECK(hint == "<src> <dst>");  // -5 is -o's value, not a positional; HEAD: same
}

TEST_CASE("format_hint flag option does not consume next token")
{
    LeafCommand root("copy", "Copy");
    root.option<fixed_string("verbose")>("--verbose", 'v', "Verbose").boolean();
    root.arg<std::string, 0>("src", "Source").required();
    root.arg<std::string, 1>("dst", "Destination").required();

    auto hint =
        HintBuilder::format(root, "--verbose src", HintConfig{HintOptionMode::None});
    CHECK(hint == "<dst>");  // src is positional 0; HEAD: same
}

TEST_CASE("format_hint unknown option token is not counted")
{
    LeafCommand root("copy", "Copy");
    root.arg<std::string, 0>("src", "Source").required();
    root.arg<std::string, 1>("dst", "Destination").required();

    auto hint =
        HintBuilder::format(root, "--unknown src", HintConfig{HintOptionMode::None});
    CHECK(hint == "<dst>");  // unknown dash token skipped; HEAD: same
}

TEST_CASE("format_hint strips whole-token quotes")
{
    App app("test", "1.0", "Test");
    auto &leaf = app.add_leaf("serve", "Serve");
    leaf.option<fixed_string("port")>("--port", 'p', "Port").integer();

    // "serve" is one token after quote stripping (owned merge path in
    // tokenize_views; the scanner does not special-case whole-token quotes).
    auto hint = HintBuilder::format(app, "\"serve\"");
    CHECK(hint.find("[INT:port]") != std::string_view::npos);
}

TEST_CASE("format_hint merges embedded quotes like the parser tokenizer")
{
    App app("test", "1.0", "Test");
    auto &leaf = app.add_leaf("serve", "Serve");
    leaf.option<fixed_string("port")>("--port", 'p', "Port").integer();

    // ser"ve" tokenizes to "serve" (interior-quote merge, owned path).
    auto hint = HintBuilder::format(app, "ser\"ve\"");
    CHECK(hint.find("[INT:port]") != std::string_view::npos);
}

TEST_CASE("format_hint counts quoted option value as a value")
{
    LeafCommand root("copy", "Copy");
    root.option<fixed_string("port")>("--port", 'p', "Port", 8080);
    root.arg<std::string, 0>("src", "Source").required();
    root.arg<std::string, 1>("dst", "Destination").required();

    CHECK(
        HintBuilder::format(root, "--port \"8080\"", HintConfig{HintOptionMode::None}) ==
        "<src> <dst>");
}

TEST_CASE("OptionInfo default_str is opt-out")
{
    LeafCommand cmd("test", "Test");
    cmd.option<fixed_string("port")>("--port", 'p', "Port", 8080);
    auto *opt = cmd.find_option_by_long("port");
    REQUIRE(opt != nullptr);

    OptionInfo with_default(*opt);
    CHECK(with_default.has_default);
    CHECK(with_default.default_str == "8080");

    OptionInfo without_default(*opt, false);
    CHECK(without_default.has_default);
    CHECK(without_default.default_str.empty());
}
