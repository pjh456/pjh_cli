#include <pjh_cli.hpp>
#include <cassert>
#include <string>
#include <string_view>
#include <vector>

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
    // ── 1. Empty args ──
    {
        App app("test", "1.0", "Empty test");
        Argv argv{"test"};
        auto r = app.parse(argv.argc(), argv.argv());
        assert(r.is_ok());
    }

    // ── 2. Simple long option ──
    {
        App app("test", "1.0", "Long option test");
        app.option<int, fixed_string("port")>("--port", "Port number");
        Argv argv{"test", "--port", "8080"};
        auto r = app.parse(argv.argc(), argv.argv());
        assert(r.is_ok());
        auto val = r.unwrap().get<int, fixed_string("port")>();
        assert(val == 8080);
    }

    // ── 3. Long option with equals ──
    {
        App app("test", "1.0", "Equals test");
        app.option<int, fixed_string("port")>("--port", "Port number");
        Argv argv{"test", "--port=8080"};
        auto r = app.parse(argv.argc(), argv.argv());
        assert(r.is_ok());
        auto val = r.unwrap().get<int, fixed_string("port")>();
        assert(val == 8080);
    }

    // ── 3b. Long option with empty equals (missing value) ──
    {
        App app("test", "1.0", "Empty equals test");
        app.option<int, fixed_string("port")>("--port", "Port number");
        Argv argv{"test", "--port="};
        auto r = app.parse(argv.argc(), argv.argv());
        assert(r.is_err());
    }

    // ── 4. Bool flag ──
    {
        App app("test", "1.0", "Flag test");
        app.option<bool, fixed_string("verbose")>("--verbose", "Verbose");
        Argv argv{"test", "--verbose"};
        auto r = app.parse(argv.argc(), argv.argv());
        assert(r.is_ok());
        auto val = r.unwrap().get<bool, fixed_string("verbose")>();
        assert(val == true);
    }

    // ── 5. Multiple options ──
    {
        App app("test", "1.0", "Multiple test");
        app.option<int, fixed_string("port")>("--port", "Port");
        app.option<bool, fixed_string("verbose")>("--verbose", "Verbose");
        Argv argv{"test", "--port", "8080", "--verbose"};
        auto r = app.parse(argv.argc(), argv.argv());
        assert(r.is_ok());
        auto &ctx = r.unwrap();
        auto port = ctx.get<int, fixed_string("port")>();
        assert(port == 8080);
        auto verb = ctx.get<bool, fixed_string("verbose")>();
        assert(verb == true);
    }

    // ── 6. Short option with next token value ──
    {
        App app("test", "1.0", "Short test");
        app.option<int, fixed_string("port")>("--port", 'p', "Port");
        Argv argv{"test", "-p", "8080"};
        auto r = app.parse(argv.argc(), argv.argv());
        assert(r.is_ok());
        auto val = r.unwrap().get<int, fixed_string("port")>();
        assert(val == 8080);
    }

    // ── 7. Short option with attached value ──
    {
        App app("test", "1.0", "Attached test");
        app.option<int, fixed_string("port")>("--port", 'p', "Port");
        Argv argv{"test", "-p8080"};
        auto r = app.parse(argv.argc(), argv.argv());
        assert(r.is_ok());
        auto val = r.unwrap().get<int, fixed_string("port")>();
        assert(val == 8080);
    }

    // ── 8. Multiple short flags ──
    {
        App app("test", "1.0", "Short flags test");
        app.option<bool, fixed_string("a")>("--a", 'a', "Flag A");
        app.option<bool, fixed_string("b")>("--b", 'b', "Flag B");
        app.option<bool, fixed_string("c")>("--c", 'c', "Flag C");
        Argv argv{"test", "-abc"};
        auto r = app.parse(argv.argc(), argv.argv());
        assert(r.is_ok());
        auto &ctx = r.unwrap();
        auto a = ctx.get<bool, fixed_string("a")>();
        auto b = ctx.get<bool, fixed_string("b")>();
        auto c = ctx.get<bool, fixed_string("c")>();
        assert(a == true);
        assert(b == true);
        assert(c == true);
    }

    // ── 9. Defaults applied when option not provided ──
    {
        App app("test", "1.0", "Default test");
        app.option<int, fixed_string("port")>("--port", "Port", 3000);
        Argv argv{"test"};
        auto r = app.parse(argv.argc(), argv.argv());
        assert(r.is_ok());
        auto val = r.unwrap().get<int, fixed_string("port")>();
        assert(val == 3000);
    }

    // ── 10. Option value overrides default ──
    {
        App app("test", "1.0", "Override test");
        app.option<int, fixed_string("port")>("--port", "Port", 3000);
        Argv argv{"test", "--port", "8080"};
        auto r = app.parse(argv.argc(), argv.argv());
        assert(r.is_ok());
        auto val = r.unwrap().get<int, fixed_string("port")>();
        assert(val == 8080);
    }

    // ── 11. Unknown long option ──
    {
        App app("test", "1.0", "Unknown test");
        Argv argv{"test", "--bogus"};
        auto r = app.parse(argv.argc(), argv.argv());
        assert(r.is_err());
    }

    // ── 12. Unknown short option ──
    {
        App app("test", "1.0", "Unknown short test");
        Argv argv{"test", "-x"};
        auto r = app.parse(argv.argc(), argv.argv());
        assert(r.is_err());
    }

    // ── 13. Missing value for long option ──
    {
        App app("test", "1.0", "Missing value test");
        app.option<int, fixed_string("port")>("--port", "Port");
        Argv argv{"test", "--port"};
        auto r = app.parse(argv.argc(), argv.argv());
        assert(r.is_err());
    }

    // ── 14. Missing value for short option ──
    {
        App app("test", "1.0", "Missing short value test");
        app.option<int, fixed_string("port")>("--port", 'p', "Port");
        Argv argv{"test", "-p"};
        auto r = app.parse(argv.argc(), argv.argv());
        assert(r.is_err());
    }

    // ── 15. Required option missing ──
    {
        App app("test", "1.0", "Required opt test");
        app.option<int, fixed_string("port")>("--port", "Port").required();
        Argv argv{"test"};
        auto r = app.parse(argv.argc(), argv.argv());
        assert(r.is_err());
    }

    // ── 16. Required positional arg missing ──
    {
        App app("test", "1.0", "Required arg test");
        app.arg<std::string, 0>("file", "Input file").required();
        Argv argv{"test"};
        auto r = app.parse(argv.argc(), argv.argv());
        assert(r.is_err());
    }

    // ── 17. Positional args ──
    {
        App app("test", "1.0", "Positional test");
        app.arg<std::string, 0>("source", "Source file");
        app.arg<std::string, 1>("dest", "Destination");
        Argv argv{"test", "in.txt", "out.txt"};
        auto r = app.parse(argv.argc(), argv.argv());
        assert(r.is_ok());
        auto &ctx = r.unwrap();
        auto s0 = ctx.get<std::string, 0>();
        auto s1 = ctx.get<std::string, 1>();
        assert(s0 == "in.txt");
        assert(s1 == "out.txt");
    }

    // ── 18. Mixed options and positional args ──
    {
        App app("test", "1.0", "Mixed test");
        app.option<bool, fixed_string("verbose")>("--verbose", 'v', "Verbose");
        app.arg<std::string, 0>("file", "Input file");
        Argv argv{"test", "--verbose", "data.txt"};
        auto r = app.parse(argv.argc(), argv.argv());
        assert(r.is_ok());
        auto &ctx = r.unwrap();
        auto verb = ctx.get<bool, fixed_string("verbose")>();
        assert(verb == true);
        auto f = ctx.get<std::string, 0>();
        assert(f == "data.txt");
    }

    // ── 19. Double dash separator ──
    {
        App app("test", "1.0", "Double dash test");
        app.option<bool, fixed_string("verbose")>("--verbose", 'v', "Verbose");
        app.arg<std::string, 0>("file", "Input file");
        Argv argv{"test", "--verbose", "--", "--file=foo"};
        auto r = app.parse(argv.argc(), argv.argv());
        assert(r.is_ok());
        auto &ctx = r.unwrap();
        auto verb = ctx.get<bool, fixed_string("verbose")>();
        assert(verb == true);
        auto f = ctx.get<std::string, 0>();
        assert(f == "--file=foo");
    }

    // ── 20. Subcommand matching ──
    {
        App app("test", "1.0", "Subcommand test");
        auto &serve = app.add_command("serve", "Start server");
        serve.option<int, fixed_string("port")>("--port", 'p', "Port");
        Argv argv{"test", "serve", "--port", "8080"};
        auto r = app.parse(argv.argc(), argv.argv());
        assert(r.is_ok());
        auto &ctx = r.unwrap();
        auto port = ctx.get<int, fixed_string("port")>();
        assert(port == 8080);
        assert(ctx.matched_path() == "serve");
    }

    // ── 21. Deep subcommand nesting ──
    {
        App app("test", "1.0", "Nested test");
        auto &db = app.add_command("db", "Database commands");
        auto &migrate = db.add_command("migrate", "Run migrations");
        migrate.option<std::string, fixed_string("name")>("--name", 'n', "Migration name");
        Argv argv{"test", "db", "migrate", "--name", "v2"};
        auto r = app.parse(argv.argc(), argv.argv());
        assert(r.is_ok());
        auto &ctx = r.unwrap();
        auto n = ctx.get<std::string, fixed_string("name")>();
        assert(n == "v2");
        assert(ctx.matched_path() == "migrate");
    }

    // ── 22. Disabled subcommand skipped ──
    {
        App app("test", "1.0", "Disabled test");
        auto &active = app.add_command("active", "Available");
        active.option<bool, fixed_string("x")>("--x", 'x', "Flag");
        app.add_command("disabled", "Unavailable")
            .enabled([]
                     { return false; });
        Argv argv{"test", "active", "--x"};
        auto r = app.parse(argv.argc(), argv.argv());
        assert(r.is_ok());
        auto x = r.unwrap().get<bool, fixed_string("x")>();
        assert(x == true);
    }

    // ── 23. Type conversion failure ──
    {
        App app("test", "1.0", "Conversion test");
        app.option<int, fixed_string("port")>("--port", "Port");
        Argv argv{"test", "--port", "notanumber"};
        auto r = app.parse(argv.argc(), argv.argv());
        assert(r.is_err());
    }

    // ── 24. Extra positional args (beyond registered, default Ignore) ──
    {
        App app("test", "1.0", "Extra args test");
        app.arg<std::string, 0>("file", "Input file");
        Argv argv{"test", "a.txt", "b.txt", "c.txt"};
        auto r = app.parse(argv.argc(), argv.argv());
        assert(r.is_ok());
        auto f = r.unwrap().get<std::string, 0>();
        assert(f == "a.txt");
    }

    // ── 25. Extra positional args with Error policy ──
    {
        App app("test", "1.0", "Strict error test");
        app.set_extra_args(ExtraArgsPolicy::Error);
        app.arg<std::string, 0>("file", "Input file");
        Argv argv{"test", "a.txt", "b.txt"};
        auto r = app.parse(argv.argc(), argv.argv());
        assert(r.is_err());
    }

    // ── 26. Extra positional args with Store policy ──
    {
        App app("test", "1.0", "Store test");
        app.set_extra_args(ExtraArgsPolicy::Store);
        app.arg<std::string, 0>("file", "Input file");
        Argv argv{"test", "a.txt", "b.txt", "c.txt"};
        auto r = app.parse(argv.argc(), argv.argv());
        assert(r.is_ok());
        auto &ctx = r.unwrap();
        assert((ctx.get<std::string, 0>() == "a.txt"));
        auto extra = ctx.extra_args();
        assert(extra.size() == 2);
        assert(extra[0] == "b.txt");
        assert(extra[1] == "c.txt");
    }

    // ── 27. Store policy with no extra args ──
    {
        App app("test", "1.0", "Store no extra");
        app.set_extra_args(ExtraArgsPolicy::Store);
        app.arg<std::string, 0>("file", "Input file");
        Argv argv{"test", "a.txt"};
        auto r = app.parse(argv.argc(), argv.argv());
        assert(r.is_ok());
        assert(r.unwrap().extra_args().empty());
    }

    return 0;
}
