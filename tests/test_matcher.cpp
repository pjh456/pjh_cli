#include <doctest/doctest.h>

#include <initializer_list>
#include <iostream>
#include <pjh_cli/app.hpp>
#include <pjh_cli/command/base_command.hpp>
#include <pjh_cli/command/leaf_command.hpp>
#include <pjh_cli/core/error.hpp>
#include <pjh_cli/core/fixed_string.hpp>
#include <pjh_cli/format/help_formatter.hpp>
#include <pjh_cli/format/matcher.hpp>
#include <pjh_cli/parse/matched_path_resolver.hpp>
#include <pjh_cli/parse/subcommand_resolver.hpp>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

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

    class PlainCommand : public BaseCommand
    {
    public:
        using BaseCommand::BaseCommand;
    };

    /// @brief Populate @p app with a root-level completer option, a serve leaf
    ///        with --port, a sibling server leaf, and a hidden secret leaf.
    void populate_completion_app(App &app)
    {
        app.option<fixed_string("color")>("--color", 'c', "Color")
            .str()
            .completer(
                []() -> std::vector<std::string> { return {"red", "green", "blue"}; });
        app.option<fixed_string("verbose")>("--verbose", 'v', "Verbose").boolean();
        auto &serve = app.add_leaf("serve", "Serve");
        serve.option<fixed_string("port")>("--port", 'p', "Port").integer();
        app.add_leaf("server", "Server");
        app.add_leaf("secret", "Secret").set_visibility(Visibility::Hidden);
    }
}

static_assert(
    !noexcept(edit_distance(std::string_view{}, std::string_view{})),
    "edit_distance allocates; must not be noexcept");

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

TEST_CASE("command-layer matcher re-exported by format/matcher.hpp")
{
    App app("test", "1.0", "Re-export test");
    app.add_leaf("server", "Start server");
    app.add_leaf("serve", "Serve");

    CHECK(edit_distance("serve", "server") == 1);
    auto matches = fuzzy_find_subcommands(app, "server", 2);
    CHECK(!matches.empty());
    CHECK(matches[0].command->name() == "server");
    auto names = list_subcommands(app);
    CHECK(names.size() == 2);
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

TEST_CASE("complete_candidates on a non-branch non-leaf command is empty")
{
    PlainCommand plain("plain", "Plain");
    CHECK(complete(plain, "").empty());
    CHECK(complete(plain, "--x").empty());
    CHECK(complete(plain, "-").empty());
}

TEST_CASE("format_usage")
{
    LeafCommand app("test", "Test app");
    app.option<fixed_string("port")>("--port", 'p', "Port", 8080);
    app.option<fixed_string("verbose")>("--verbose", 'v', "Verbose").boolean();
    app.arg<std::string, 0>("source", "Source file").required();
    app.arg<std::string, 1>("dest", "Destination");

    auto usage = HelpFormatter::format_usage(app, "test");
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

    auto help = HelpFormatter::format_help(app, "test");
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

    auto help = HelpFormatter::format_help(app, "test");
    CHECK(help.find("Usage:") != std::string_view::npos);
    CHECK(help.find("Subcommands:") != std::string_view::npos);
    CHECK(help.find("serve") != std::string_view::npos);
}

TEST_CASE("format_help derives full path when program name omitted")
{
    App app("test", "1.0", "App");
    auto &container = app.add_branch("container", "Container");
    auto &start = container.add_leaf("start", "Start");

    CHECK(HelpFormatter::format_help(start).starts_with("Usage: test container start"));
    CHECK(HelpFormatter::format_usage(start).starts_with("Usage: test container start"));
    CHECK(HelpFormatter::format_help(start, "custom").starts_with("Usage: custom"));
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
    CHECK(MatchedPathResolver::to_path_string(r.unwrap().matched_command()) == "server");
}

TEST_CASE("parse_fuzzy ambiguous reports AmbiguousCommandError")
{
    App app("test", "1.0", "Ambiguous");
    app.add_leaf("start", "Start");
    app.add_leaf("stop", "Stop");

    Argv argv{"test", "st"};
    auto r = app.parse_fuzzy(argv.argc(), argv.argv());
    CHECK(r.is_err());
    auto &err = r.unwrap_err();
    CHECK(std::holds_alternative<AmbiguousCommandError>(err.info()));
    auto msg = std::string_view(err.what());
    CHECK(msg.find("ambiguous command 'st'") != std::string_view::npos);
    CHECK(msg.find("start") != std::string_view::npos);
    CHECK(msg.find("stop") != std::string_view::npos);
}

TEST_CASE("parse exact mode ambiguous name is unknown command")
{
    App app("test", "1.0", "Exact ambiguous");
    app.add_leaf("start", "Start");
    app.add_leaf("stop", "Stop");

    Argv argv{"test", "st"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_err());
    auto &err = r.unwrap_err();
    CHECK_FALSE(std::holds_alternative<AmbiguousCommandError>(err.info()));
    CHECK(std::holds_alternative<UnknownCommandError>(err.info()));
    auto msg = std::string_view(err.what());
    CHECK(msg.find("unknown command: 'st'") != std::string_view::npos);
    CHECK(msg.find("stop, start") != std::string_view::npos);
}

TEST_CASE("find_subcommand_match reports ambiguity candidates")
{
    App app("test", "1.0", "Resolver");
    app.add_leaf("start", "Start");
    app.add_leaf("stop", "Stop");

    bool disabled = false;
    std::vector<std::string> ambiguous;
    auto *m =
        SubcommandResolver::find_subcommand_match(app, "st", 3, disabled, ambiguous);
    CHECK(m == nullptr);
    CHECK_FALSE(disabled);
    REQUIRE(ambiguous.size() == 2);
    CHECK(ambiguous[0] == "stop");
    CHECK(ambiguous[1] == "start");
}

TEST_CASE("parse_fuzzy ambiguous nested branch")
{
    App app("test", "1.0", "Nested ambiguous");
    auto &db = app.add_branch("db", "Database");
    db.add_leaf("migrate", "Migrate");
    db.add_leaf("mirror", "Mirror");

    Argv argv{"test", "db", "migrat"};
    auto r = app.parse_fuzzy(argv.argc(), argv.argv());
    CHECK(r.is_err());
    CHECK(std::holds_alternative<AmbiguousCommandError>(r.unwrap_err().info()));
}

TEST_CASE("parse_fuzzy no match")
{
    App app("test", "1.0", "No fuzzy match");
    app.add_leaf("server", "Server");

    Argv argv{"test", "zzzzz"};
    auto r = app.parse_fuzzy(argv.argc(), argv.argv());
    CHECK(r.is_err());
    CHECK(
        std::string_view(r.unwrap_err().what()).find("unknown command: 'zzzzz'") !=
        std::string_view::npos);
}

TEST_CASE("parse_fuzzy multi-candidate ambiguous reports ambiguous command")
{
    App app("test", "1.0", "Multi fuzzy");
    app.add_leaf("start", "Start server");
    app.add_leaf("stop", "Stop server");

    Argv argv{"test", "st"};
    auto r = app.parse_fuzzy(argv.argc(), argv.argv());
    CHECK(r.is_err());
    // Both "start" and "stop" are within edit distance 3 from "st", so fuzzy
    // matching is ambiguous and neither is matched.  The parser reports an
    // ambiguous command listing the candidates.
    auto &err = r.unwrap_err();
    CHECK(std::holds_alternative<AmbiguousCommandError>(err.info()));
    auto msg = std::string_view(err.what());
    CHECK(msg.find("ambiguous command 'st'") != std::string_view::npos);
    CHECK(msg.find("start") != std::string_view::npos);
    CHECK(msg.find("stop") != std::string_view::npos);
}

TEST_CASE("collect_help respects visibility filter")
{
    App app("test", "1.0", "Visibility");
    app.add_leaf("cli_only", "CLI only").set_visibility(Visibility::Cli);
    app.add_leaf("repl_only", "REPL only").set_visibility(Visibility::Repl);
    app.add_leaf("both_cmd", "Both").set_visibility(Visibility::Both);
    app.add_leaf("hidden_cmd", "Hidden").set_visibility(Visibility::Hidden);

    auto all = HelpFormatter::collect_help(app, "test", Visibility::Both);
    CHECK(all.subcommands.size() == 3);

    auto cli = HelpFormatter::collect_help(app, "test", Visibility::Cli);
    CHECK(cli.subcommands.size() == 2);

    auto repl = HelpFormatter::collect_help(app, "test", Visibility::Repl);
    CHECK(repl.subcommands.size() == 2);

    auto none = HelpFormatter::collect_help(app, "test", static_cast<Visibility>(0));
    CHECK(none.subcommands.empty());
}

TEST_CASE("collect_help includes option metadata")
{
    LeafCommand cmd("test", "Test app");
    cmd.option<fixed_string("port")>("--port", 'p', "Port number")
        .integer().default_value(8080).required();
    cmd.option<fixed_string("verbose")>("--verbose", 'v', "Verbose output").boolean();
    cmd.arg<std::string, 0>("file", "Input file").required();

    auto info = HelpFormatter::collect_help(cmd, "test");
    CHECK(info.program_name == "test");
    CHECK(info.description == "Test app");
    CHECK(info.options.size() == 2);
    CHECK(info.args.size() == 1);
    CHECK(info.args[0].name == "file");
    CHECK(info.args[0].is_required);
}

TEST_CASE("format_help shows env metadata")
{
    LeafCommand cmd("test", "Test app");
    cmd.option<fixed_string("host")>("--host", 'H', "Host").str().env("APP_HOST");

    auto help = HelpFormatter::format_help(cmd, "test");
    CHECK(help.find("(env: APP_HOST)") != std::string::npos);
}

TEST_CASE("format_help shows negatable metadata")
{
    LeafCommand cmd("test", "Test app");
    cmd.option<fixed_string("compress")>("--compress", 'c', "Compress")
        .boolean()
        .negatable();

    auto help = HelpFormatter::format_help(cmd, "test");
    CHECK(help.find("(negatable)") != std::string::npos);
}

TEST_CASE("format_help shows counting metadata")
{
    LeafCommand cmd("test", "Test app");
    cmd.option<fixed_string("verbose")>("--verbose", 'v', "Verbose").count();

    auto help = HelpFormatter::format_help(cmd, "test");
    CHECK(help.find("(counting)") != std::string::npos);
}

TEST_CASE("format_help shows repeatable metadata")
{
    LeafCommand cmd("test", "Test app");
    cmd.option<fixed_string("include")>("--include", 'I', "Include path")
        .path()
        .repeatable();

    auto help = HelpFormatter::format_help(cmd, "test");
    CHECK(help.find("(repeatable)") != std::string::npos);
    CHECK(help.find("[...]") == std::string::npos);
}

TEST_CASE("format_help orders option annotations")
{
    LeafCommand cmd("test", "Test app");
    cmd.option<fixed_string("host")>("--host", 'H', "Host")
        .str()
        .required()
        .default_value("x")
        .env("V")
        .repeatable();
    cmd.option<fixed_string("compress")>("--compress", 'c', "Compress")
        .boolean()
        .negatable();

    auto help = HelpFormatter::format_help(cmd, "test");
    CHECK(
        help.find(" (required) (env: V) (default: x) (repeatable)") != std::string::npos);
    CHECK(help.find(" (negatable)") != std::string::npos);
}

TEST_CASE("collect_help exposes option metadata")
{
    LeafCommand cmd("test", "Test app");
    cmd.option<fixed_string("host")>("--host", 'H', "Host").str().env("APP_HOST");
    cmd.option<fixed_string("compress")>("--compress", 'c', "Compress")
        .boolean()
        .negatable();
    cmd.option<fixed_string("verbose")>("--verbose", 'v', "Verbose").count();
    cmd.option<fixed_string("include")>("--include", 'I', "Include").path().repeatable();

    auto info = HelpFormatter::collect_help(cmd, "test");
    REQUIRE(info.options.size() == 4);
    CHECK(info.options[0].env_var == "APP_HOST");
    CHECK(info.options[1].is_negatable);
    CHECK(info.options[2].is_counting);
    CHECK(info.options[3].is_repeatable);
}

TEST_CASE("format_usage drops repeatable marker")
{
    LeafCommand cmd("test", "Test app");
    cmd.option<fixed_string("include")>("--include", 'I', "Include path")
        .path()
        .repeatable();

    auto usage = HelpFormatter::format_usage(cmd, "test");
    CHECK(usage.find("--include INCLUDE") != std::string::npos);
    CHECK(usage.find("[...]") == std::string::npos);
}

// ──────────────────────────────────────────
//  Alias tests
// ──────────────────────────────────────────

TEST_CASE("fuzzy find matches alias name")
{
    App app("test", "1.0", "Fuzzy alias");
    auto &cmd = app.add_leaf("server", "Start server");
    cmd.alias("sr");

    auto exact = fuzzy_find_subcommands(app, "sever", 2);
    CHECK(!exact.empty());
    CHECK(exact[0].command->name() == "server");
}

TEST_CASE("complete matches alias prefix")
{
    App app("test", "1.0", "Complete alias");
    auto &cmd = app.add_leaf("list", "List items");
    cmd.alias("ls");

    auto candidates = complete(app, "l");
    bool has_ls = false;
    for (const auto &c : candidates)
        if (c == "ls")
            has_ls = true;
    CHECK(has_ls);
}

TEST_CASE("format_help shows aliases in subcommands section")
{
    App app("test", "1.0", "App with aliases");
    auto &cmd = app.add_leaf("list", "List items");
    cmd.alias("ls").alias("show");

    auto help = HelpFormatter::format_help(app, "test");
    CHECK(help.find("Subcommands:") != std::string_view::npos);
    CHECK(help.find("list") != std::string_view::npos);
    CHECK(help.find("ls, show") != std::string_view::npos);
}

TEST_CASE("list_subcommands does not include aliases")
{
    App app("test", "1.0", "List no aliases");
    app.add_leaf("list", "List items").alias("ls");
    app.add_leaf("config", "Config").alias("cfg");

    auto names = list_subcommands(app);
    CHECK(names.size() == 2);
    CHECK(std::find(names.begin(), names.end(), "list") != names.end());
    CHECK(std::find(names.begin(), names.end(), "config") != names.end());
    // Aliases should not appear as separate entries
    CHECK(std::find(names.begin(), names.end(), "ls") == names.end());
    CHECK(std::find(names.begin(), names.end(), "cfg") == names.end());
}

// ──────────────────────────────────────────
//  Option-value completion
// ──────────────────────────────────────────

TEST_CASE("complete_value_candidates invokes completer")
{
    App app("test", "1.0", "Value complete");
    populate_completion_app(app);
    auto *opt = app.find_option_by_long("color");
    REQUIRE(opt != nullptr);

    auto red = complete_value_candidates(*opt, "r");
    REQUIRE(red.size() == 1);
    CHECK(red[0].display == "red");

    auto green = complete_value_candidates(*opt, "g");
    REQUIRE(green.size() == 1);
    CHECK(green[0].display == "green");

    auto all = complete_value_candidates(*opt, "");
    REQUIRE(all.size() == 3);
    CHECK(all[0].display == "blue");
    CHECK(all[1].display == "green");
    CHECK(all[2].display == "red");

    LeafCommand dup("dup", "Dup");
    dup.option<fixed_string("d")>("--d", "D")
        .str()
        .completer([]() -> std::vector<std::string> { return {"red", "red", "green"}; });
    auto *dopt = dup.find_option_by_long("d");
    REQUIRE(dopt != nullptr);
    auto dedup = complete_value_candidates(*dopt, "");
    REQUIRE(dedup.size() == 2);
    CHECK(dedup[0].display == "green");
    CHECK(dedup[1].display == "red");
}

TEST_CASE("complete_value_candidates empty without completer")
{
    App app("test", "1.0", "No completer");
    populate_completion_app(app);
    auto *opt = app.find_subcommand("serve")->find_option_by_long("port");
    REQUIRE(opt != nullptr);
    CHECK(complete_value_candidates(*opt, "").empty());
}

TEST_CASE("complete_line completes option value after long option")
{
    App app("test", "1.0", "Complete line");
    populate_completion_app(app);

    auto r = complete_line(app, "serve --color r", 15);
    REQUIRE(r.size() == 1);
    CHECK(r[0].display == "red");
}

TEST_CASE("complete_line completes option value after inline equals")
{
    App app("test", "1.0", "Complete line");
    populate_completion_app(app);

    auto r = complete_line(app, "--color=g", 9);
    REQUIRE(r.size() == 1);
    CHECK(r[0].display == "green");
}

TEST_CASE("complete_line completes option value after short option")
{
    App app("test", "1.0", "Complete line");
    populate_completion_app(app);

    auto r = complete_line(app, "serve -c r", 10);
    REQUIRE(r.size() == 1);
    CHECK(r[0].display == "red");
}

TEST_CASE("complete_line completes compact short option value")
{
    App app("test", "1.0", "Complete line");
    populate_completion_app(app);

    auto r = complete_line(app, "serve -cred", 11);
    REQUIRE(r.size() == 1);
    CHECK(r[0].display == "red");
}

TEST_CASE("complete_line_result reports the matched prefix length")
{
    App app("test", "1.0", "Complete line");
    populate_completion_app(app);

    auto name = complete_line_result(app, "ser", 3);
    CHECK(name.prefix_len == 3);
    REQUIRE(name.candidates.size() == 2);

    auto separate = complete_line_result(app, "serve --color g", 15);
    CHECK(separate.prefix_len == 1);
    REQUIRE(separate.candidates.size() == 1);
    CHECK(separate.candidates[0].display == "green");

    auto inline_eq = complete_line_result(app, "--color=gr", 10);
    CHECK(inline_eq.prefix_len == 2);
    REQUIRE(inline_eq.candidates.size() == 1);
    CHECK(inline_eq.candidates[0].display == "green");

    auto inline_empty = complete_line_result(app, "--color=", 8);
    CHECK(inline_empty.prefix_len == 0);
    REQUIRE(inline_empty.candidates.size() == 3);

    auto compact = complete_line_result(app, "serve -cgr", 10);
    CHECK(compact.prefix_len == 2);
    REQUIRE(compact.candidates.size() == 1);
    CHECK(compact.candidates[0].display == "green");
}

TEST_CASE("complete_line completes subcommand after trailing space")
{
    App app("test", "1.0", "Complete line");
    populate_completion_app(app);

    auto r = complete_line(app, "ser", 3);
    bool has_serve = false, has_server = false;
    for (const auto &c : r)
    {
        if (c.display == "serve")
            has_serve = true;
        if (c.display == "server")
            has_server = true;
    }
    CHECK(has_serve);
    CHECK(has_server);
}

TEST_CASE("complete_line completes option names in subcommand context")
{
    App app("test", "1.0", "Complete line");
    populate_completion_app(app);

    // Prefix "--po" matches no ancestor option, so this case no longer pins
    // node-locality; ancestor coverage is in the cases below.
    auto r = complete_line(app, "serve --po", 10);
    REQUIRE(r.size() == 1);
    CHECK(r[0].display == "--port");
    for (const auto &c : r)
    {
        CHECK(c.display != "--verbose");
        CHECK(c.display != "--color");
    }
}

TEST_CASE("complete_line does not value-complete a flag")
{
    App app("test", "1.0", "Complete line");
    populate_completion_app(app);

    auto r = complete_line(app, "serve --verbose r", 17);
    CHECK(r.empty());
}

TEST_CASE("complete_line respects visibility")
{
    App app("test", "1.0", "Complete line");
    populate_completion_app(app);

    CHECK(complete_line(app, "sec", 3, Visibility::Both).empty());
}

TEST_CASE("complete_line handles double-dash barrier")
{
    App app("test", "1.0", "Complete line");
    populate_completion_app(app);

    auto r = complete_line(app, "serve -- --color r", 18);
    CHECK(r.empty());
}

// ──────────────────────────────────────────
//  Ancestor option visibility
// ──────────────────────────────────────────

TEST_CASE("format_help shows inherited options section")
{
    App app("test", "1.0", "App");
    app.option<fixed_string("verbose")>("--verbose", 'v', "Verbose").boolean();
    auto &serve = app.add_leaf("serve", "Serve");
    serve.option<fixed_string("port")>("--port", 'p', "Port").integer();

    auto help = HelpFormatter::format_help(serve);
    CHECK(help.find("Inherited Options:") != std::string::npos);
    CHECK(help.find("--verbose") != std::string::npos);
    CHECK(help.find("Verbose") != std::string::npos);
    // Usage line stays node-local.
    CHECK(help.substr(0, help.find('\n')).find("--verbose") == std::string::npos);
}

TEST_CASE("format_help inherited options are nearest-first")
{
    App app("test", "1.0", "App");
    app.option<fixed_string("root")>("--root", "Root").boolean();
    auto &mid = app.add_branch("mid", "Mid");
    mid.option<fixed_string("mid")>("--mid", "Mid opt").boolean();
    auto &leaf = mid.add_leaf("leaf", "Leaf");
    leaf.option<fixed_string("leaf")>("--leaf", "Leaf opt").boolean();

    auto help = HelpFormatter::format_help(leaf);
    auto mid_pos = help.find("--mid");
    auto root_pos = help.find("--root");
    REQUIRE(mid_pos != std::string::npos);
    REQUIRE(root_pos != std::string::npos);
    CHECK(mid_pos < root_pos);
}

TEST_CASE("format_help omits shadowed ancestor option")
{
    App app("test", "1.0", "App");
    app.option<fixed_string("opt")>("--opt", "Root").boolean();
    auto &child = app.add_leaf("child", "Child");
    child.option<fixed_string("opt")>("--opt", "Leaf").boolean();

    auto help = HelpFormatter::format_help(child);
    CHECK(help.find("Inherited Options:") == std::string::npos);
}

TEST_CASE("format_help renders partially shadowed ancestor short")
{
    App app("test", "1.0", "App");
    app.option<fixed_string("opt")>("--opt", 'o', "Root").boolean();
    auto &child = app.add_leaf("child", "Child");
    child.option<fixed_string("opt")>("--opt", "Leaf").boolean();

    auto help = HelpFormatter::format_help(child);
    auto pos = help.find("Inherited Options:");
    REQUIRE(pos != std::string::npos);
    auto inherited = help.substr(pos);
    CHECK(inherited.find("-o") != std::string::npos);
    CHECK(inherited.find("--opt") == std::string::npos);
}

TEST_CASE("collect_help separates current and inherited options")
{
    App app("test", "1.0", "App");
    app.option<fixed_string("verbose")>("--verbose", 'v', "Verbose").boolean();
    auto &serve = app.add_leaf("serve", "Serve");
    serve.option<fixed_string("port")>("--port", 'p', "Port").integer();

    auto info = HelpFormatter::collect_help(serve, "test serve");
    REQUIRE(info.options.size() == 1);
    CHECK(info.options[0].long_name == "port");
    REQUIRE(info.inherited_options.size() == 1);
    CHECK(info.inherited_options[0].long_name == "verbose");
}

TEST_CASE("complete_line offers ancestor option names in subcommand context")
{
    App app("test", "1.0", "Complete line");
    populate_completion_app(app);

    auto r = complete_line(app, "serve --", 8);
    bool has_color = false, has_port = false, has_verbose = false;
    for (const auto &c : r)
    {
        if (c.display == "--color")
            has_color = true;
        if (c.display == "--port")
            has_port = true;
        if (c.display == "--verbose")
            has_verbose = true;
    }
    CHECK(has_color);
    CHECK(has_port);
    CHECK(has_verbose);
}

TEST_CASE("complete_line completes an ancestor long option after descent")
{
    App app("test", "1.0", "Complete line");
    populate_completion_app(app);

    auto r = complete_line(app, "serve --v", 9);
    REQUIRE(r.size() == 1);
    CHECK(r[0].display == "--verbose");
}

TEST_CASE("complete_line offers ancestor short options after descent")
{
    App app("test", "1.0", "Complete line");
    populate_completion_app(app);

    auto r = complete_line(app, "serve -", 7);
    bool has_c = false, has_p = false, has_v = false;
    for (const auto &c : r)
    {
        if (c.display == "-c")
            has_c = true;
        if (c.display == "-p")
            has_p = true;
        if (c.display == "-v")
            has_v = true;
    }
    CHECK(has_c);
    CHECK(has_p);
    CHECK(has_v);
}

TEST_CASE("complete_candidates deduplicates a shadowed ancestor option")
{
    App app("test", "1.0", "Dedup");
    app.option<fixed_string("opt")>("--opt", "Root").boolean();
    auto &son = app.add_leaf("son", "Son");
    son.option<fixed_string("opt")>("--opt", "Leaf").boolean();

    auto candidates = complete(son, "--");
    REQUIRE(candidates.size() == 1);
    CHECK(candidates[0] == "--opt");
}
