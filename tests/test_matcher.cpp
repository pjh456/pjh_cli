#include <pjh_cli.hpp>
#include <pjh_cli/matcher.hpp>
#include <cassert>
#include <string>
#include <string_view>

using namespace pjh::cli;

struct Argv
{
    std::vector<std::string> storage;
    std::vector<char *> ptrs;

    Argv(std::initializer_list<std::string> list)
        : storage(list)
    {
        for (auto &s : storage)
            ptrs.push_back(s.data());
    }

    int argc() const { return static_cast<int>(ptrs.size()); }
    char **argv() { return ptrs.data(); }
};

int main()
{
    // ── edit_distance ──
    {
        assert(edit_distance("", "") == 0);
        assert(edit_distance("abc", "abc") == 0);
        assert(edit_distance("abc", "ab") == 1);
        assert(edit_distance("abc", "abcd") == 1);
        assert(edit_distance("abc", "abd") == 1);
        assert(edit_distance("abc", "xyz") == 3);
        assert(edit_distance("serve", "server") == 1);
        assert(edit_distance("start", "stop") == 3);
    }

    // ── fuzzy_find_subcommands ──
    {
        App app("test", "1.0", "Fuzzy test");
        app.add_command("server", "Start server");
        app.add_command("config", "Configuration");
        app.add_command("deploy", "Deploy app");
        app.add_command("help", "Show help");

        // Exact match (distance=0) with max_distance=2
        {
            auto matches = fuzzy_find_subcommands(app, "server", 2);
            assert(matches.size() >= 1);
            assert(matches[0].distance == 0);
            assert(matches[0].command->name() == "server");
        }

        // Fuzzy match: "serv" → "server" (distance 1)
        {
            auto matches = fuzzy_find_subcommands(app, "serv", 2);
            assert(!matches.empty());
            assert(matches[0].command->name() == "server");
            assert(matches[0].distance <= 2);
        }

        // No match beyond threshold
        {
            auto matches = fuzzy_find_subcommands(app, "zzzzz", 2);
            assert(matches.empty());
        }
    }

    // ── fuzzy_find respects enabled + visibility ──
    {
        App app("test", "1.0", "Visibility test");
        app.add_command("visible", "Visible");
        auto &hidden = app.add_command("hidden", "Hidden");
        hidden.set_visibility(Visibility::Hidden);

        {
            auto matches = fuzzy_find_subcommands(app, "visible", 2);
            assert(!matches.empty());
        }
        {
            auto matches = fuzzy_find_subcommands(app, "hidden", 2);
            assert(matches.empty()); // hidden, not returned
        }
    }

    // ── list_subcommands ──
    {
        App app("test", "1.0", "List test");
        app.add_command("foo", "Foo");
        app.add_command("bar", "Bar");
        app.add_command("baz", "Baz");

        auto names = list_subcommands(app);
        assert(names.size() == 3);
    }

    // ── complete: subcommand prefix ──
    {
        App app("test", "1.0", "Complete test");
        app.add_command("server", "Server");
        app.add_command("serve", "Serve");

        {
            auto candidates = complete(app, "ser");
            assert(!candidates.empty());
            bool has_server = false, has_serve = false;
            for (const auto &c : candidates)
            {
                if (c == "server")
                    has_server = true;
                if (c == "serve")
                    has_serve = true;
            }
            assert(has_server);
            assert(has_serve);
        }

        // No match
        {
            auto candidates = complete(app, "x");
            assert(candidates.empty());
        }
    }

    // ── complete: option prefix ──
    {
        App app("test", "1.0", "Option complete");
        app.option<bool, fixed_string("verbose")>("--verbose", 'v', "Verbose");
        app.option<int, fixed_string("port")>("--port", 'p', "Port");

        // Long option prefix
        {
            auto candidates = complete(app, "--ver");
            assert(!candidates.empty());
            assert(candidates[0] == "--verbose");
        }

        // Short option prefix
        {
            auto candidates = complete(app, "-");
            bool has_v = false, has_p = false;
            for (const auto &c : candidates)
            {
                if (c == "-v")
                    has_v = true;
                if (c == "-p")
                    has_p = true;
            }
            assert(has_v);
            assert(has_p);
        }
    }

    // ── format_usage ──
    {
        App app("test", "1.0", "Test app");
        app.option<int, fixed_string("port")>("--port", 'p', "Port", 8080);
        app.option<bool, fixed_string("verbose")>("--verbose", 'v', "Verbose");
        app.arg<std::string, 0>("source", "Source file").required();
        app.arg<std::string, 1>("dest", "Destination");

        auto usage = format_usage(app, "test");
        assert(usage.find("Usage:") != std::string_view::npos);
        assert(usage.find("--port") != std::string_view::npos);
        assert(usage.find("--verbose") != std::string_view::npos);
        assert(usage.find("<source>") != std::string_view::npos);
        assert(usage.find("[dest]") != std::string_view::npos);
    }

    // ── format_help ──
    {
        App app("test", "1.0", "Test application");
        app.option<int, fixed_string("port")>("--port", 'p', "Port number", 8080);
        app.option<bool, fixed_string("verbose")>("--verbose", 'v', "Enable verbose");
        app.arg<std::string, 0>("file", "Input file").required();
        app.add_command("serve", "Start the server");

        auto help = format_help(app, "test");
        assert(help.find("Usage:") != std::string_view::npos);
        assert(help.find("Test application") != std::string_view::npos);
        assert(help.find("Options:") != std::string_view::npos);
        assert(help.find("Arguments:") != std::string_view::npos);
        assert(help.find("Subcommands:") != std::string_view::npos);
        assert(help.find("serve") != std::string_view::npos);
    }

    // ── parse_fuzzy: exact match still works ──
    {
        App app("test", "1.0", "Fuzzy parse");
        auto &serve = app.add_command("server", "Server");
        serve.option<int, fixed_string("port")>("--port", 'p', "Port", 8080);

        Argv argv{"test", "server", "--port", "3000"};
        auto r = app.parse_fuzzy(argv.argc(), argv.argv());
        assert(r.is_ok());
        auto port = r.unwrap().get<int, fixed_string("port")>();
        assert(port == 3000);
    }

    // ── parse_fuzzy: typo-tolerant matching ──
    {
        App app("test", "1.0", "Fuzzy parse typo");
        app.add_command("server", "Server");
        app.add_command("config", "Config");

        // "servr" → "server" (edit distance 1)
        Argv argv{"test", "servr"};
        auto r = app.parse_fuzzy(argv.argc(), argv.argv());
        assert(r.is_ok());
        assert(r.unwrap().matched_path() == "server");
    }

    // ── parse_fuzzy: ambiguous → no match, error ──
    {
        App app("test", "1.0", "Ambiguous");
        app.add_command("start", "Start");
        app.add_command("stop", "Stop");

        // "st" could match both, distance is high for both prefix-only
        Argv argv{"test", "st"};
        auto r = app.parse_fuzzy(argv.argc(), argv.argv());
        assert(r.is_ok()); // no subcommand matched, falls through to options/args of root
    }

    // ── parse_fuzzy: no match when too far ──
    {
        App app("test", "1.0", "No fuzzy match");
        app.add_command("server", "Server");

        Argv argv{"test", "zzzzz"};
        auto r = app.parse_fuzzy(argv.argc(), argv.argv());
        assert(r.is_ok()); // falls through, treated as positional
    }

    return 0;
}
