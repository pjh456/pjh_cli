#include <pjh_cli.hpp>
#include <doctest/doctest.h>
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

TEST_CASE("Parser empty args")
{
    App app("test", "1.0", "Empty test");
    Argv argv{"test"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_ok());
}

TEST_CASE("Parser simple long option")
{
    App app("test", "1.0", "Long option test");
    app.option<int, fixed_string("port")>("--port", "Port number");
    Argv argv{"test", "--port", "8080"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_ok());
    auto val = r.unwrap().get<int, fixed_string("port")>();
    CHECK(val == 8080);
}

TEST_CASE("Parser long option with equals value")
{
    App app("test", "1.0", "Equals test");
    app.option<int, fixed_string("port")>("--port", "Port number");
    Argv argv{"test", "--port=8080"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_ok());
    auto val = r.unwrap().get<int, fixed_string("port")>();
    CHECK(val == 8080);
}

TEST_CASE("Parser long option with empty equals")
{
    App app("test", "1.0", "Empty equals test");
    app.option<int, fixed_string("port")>("--port", "Port number");
    Argv argv{"test", "--port="};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_err());
}

TEST_CASE("Parser bool flag")
{
    App app("test", "1.0", "Flag test");
    app.option<bool, fixed_string("verbose")>("--verbose", "Verbose");
    Argv argv{"test", "--verbose"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_ok());
    auto val = r.unwrap().get<bool, fixed_string("verbose")>();
    CHECK(val == true);
}

TEST_CASE("Parser multiple options")
{
    App app("test", "1.0", "Multiple test");
    app.option<int, fixed_string("port")>("--port", "Port");
    app.option<bool, fixed_string("verbose")>("--verbose", "Verbose");
    Argv argv{"test", "--port", "8080", "--verbose"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_ok());
    auto &ctx = r.unwrap();
    auto port = ctx.get<int, fixed_string("port")>();
    CHECK(port == 8080);
    auto verb = ctx.get<bool, fixed_string("verbose")>();
    CHECK(verb == true);
}

TEST_CASE("Parser short option next token")
{
    App app("test", "1.0", "Short test");
    app.option<int, fixed_string("port")>("--port", 'p', "Port");
    Argv argv{"test", "-p", "8080"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_ok());
    auto val = r.unwrap().get<int, fixed_string("port")>();
    CHECK(val == 8080);
}

TEST_CASE("Parser short option attached value")
{
    App app("test", "1.0", "Attached test");
    app.option<int, fixed_string("port")>("--port", 'p', "Port");
    Argv argv{"test", "-p8080"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_ok());
    auto val = r.unwrap().get<int, fixed_string("port")>();
    CHECK(val == 8080);
}

TEST_CASE("Parser multiple short flags")
{
    App app("test", "1.0", "Short flags test");
    app.option<bool, fixed_string("a")>("--a", 'a', "Flag A");
    app.option<bool, fixed_string("b")>("--b", 'b', "Flag B");
    app.option<bool, fixed_string("c")>("--c", 'c', "Flag C");
    Argv argv{"test", "-abc"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_ok());
    auto &ctx = r.unwrap();
    CHECK(ctx.get<bool, fixed_string("a")>() == true);
    CHECK(ctx.get<bool, fixed_string("b")>() == true);
    CHECK(ctx.get<bool, fixed_string("c")>() == true);
}

TEST_CASE("Parser default applied when option not provided")
{
    App app("test", "1.0", "Default test");
    app.option<int, fixed_string("port")>("--port", "Port", 3000);
    Argv argv{"test"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_ok());
    auto val = r.unwrap().get<int, fixed_string("port")>();
    CHECK(val == 3000);
}

TEST_CASE("Parser option value overrides default")
{
    App app("test", "1.0", "Override test");
    app.option<int, fixed_string("port")>("--port", "Port", 3000);
    Argv argv{"test", "--port", "8080"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_ok());
    auto val = r.unwrap().get<int, fixed_string("port")>();
    CHECK(val == 8080);
}

TEST_CASE("Parser unknown long option")
{
    App app("test", "1.0", "Unknown test");
    Argv argv{"test", "--bogus"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_err());
}

TEST_CASE("Parser unknown short option")
{
    App app("test", "1.0", "Unknown short test");
    Argv argv{"test", "-x"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_err());
}

TEST_CASE("Parser missing value for long option")
{
    App app("test", "1.0", "Missing value test");
    app.option<int, fixed_string("port")>("--port", "Port");
    Argv argv{"test", "--port"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_err());
}

TEST_CASE("Parser missing value for short option")
{
    App app("test", "1.0", "Missing short value test");
    app.option<int, fixed_string("port")>("--port", 'p', "Port");
    Argv argv{"test", "-p"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_err());
}

TEST_CASE("Parser required option missing")
{
    App app("test", "1.0", "Required opt test");
    app.option<int, fixed_string("port")>("--port", "Port").required();
    Argv argv{"test"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_err());
}

TEST_CASE("Parser required positional arg missing")
{
    App app("test", "1.0", "Required arg test");
    app.arg<std::string, 0>("file", "Input file").required();
    Argv argv{"test"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_err());
}

TEST_CASE("Parser type conversion failure")
{
    App app("test", "1.0", "Conversion test");
    app.option<int, fixed_string("port")>("--port", "Port");
    Argv argv{"test", "--port", "notanumber"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_err());
}

TEST_CASE("Parser positional args")
{
    App app("test", "1.0", "Positional test");
    app.arg<std::string, 0>("source", "Source file");
    app.arg<std::string, 1>("dest", "Destination");
    Argv argv{"test", "in.txt", "out.txt"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_ok());
    auto &ctx = r.unwrap();
    CHECK(ctx.get<std::string, 0>() == "in.txt");
    CHECK(ctx.get<std::string, 1>() == "out.txt");
}

TEST_CASE("Parser mixed options and positional args")
{
    App app("test", "1.0", "Mixed test");
    app.option<bool, fixed_string("verbose")>("--verbose", 'v', "Verbose");
    app.arg<std::string, 0>("file", "Input file");
    Argv argv{"test", "--verbose", "data.txt"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_ok());
    auto &ctx = r.unwrap();
    CHECK(ctx.get<bool, fixed_string("verbose")>() == true);
    CHECK(ctx.get<std::string, 0>() == "data.txt");
}

TEST_CASE("Parser double dash separator")
{
    App app("test", "1.0", "Double dash test");
    app.option<bool, fixed_string("verbose")>("--verbose", 'v', "Verbose");
    app.arg<std::string, 0>("file", "Input file");
    Argv argv{"test", "--verbose", "--", "--file=foo"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_ok());
    auto &ctx = r.unwrap();
    CHECK(ctx.get<bool, fixed_string("verbose")>() == true);
    CHECK(ctx.get<std::string, 0>() == "--file=foo");
}

TEST_CASE("Parser subcommand matching")
{
    App app("test", "1.0", "Subcommand test");
    auto &serve = app.add_command("serve", "Start server");
    serve.option<int, fixed_string("port")>("--port", 'p', "Port");
    Argv argv{"test", "serve", "--port", "8080"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_ok());
    auto &ctx = r.unwrap();
    CHECK(ctx.get<int, fixed_string("port")>() == 8080);
    CHECK(ctx.matched_path() == "serve");
}

TEST_CASE("Parser deep subcommand nesting")
{
    App app("test", "1.0", "Nested test");
    auto &db = app.add_command("db", "Database commands");
    auto &migrate = db.add_command("migrate", "Run migrations");
    migrate.option<std::string, fixed_string("name")>("--name", 'n', "Migration name");
    Argv argv{"test", "db", "migrate", "--name", "v2"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_ok());
    auto &ctx = r.unwrap();
    CHECK(ctx.get<std::string, fixed_string("name")>() == "v2");
    CHECK(ctx.matched_path() == "migrate");
}

TEST_CASE("Parser disabled subcommand skipped")
{
    App app("test", "1.0", "Disabled test");
    auto &active = app.add_command("active", "Available");
    active.option<bool, fixed_string("x")>("--x", 'x', "Flag");
    app.add_command("disabled", "Unavailable")
        .enabled([] { return false; });
    Argv argv{"test", "active", "--x"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_ok());
    CHECK(r.unwrap().get<bool, fixed_string("x")>() == true);
}

TEST_CASE("Parser extra positional args ignore")
{
    App app("test", "1.0", "Extra args test");
    app.arg<std::string, 0>("file", "Input file");
    Argv argv{"test", "a.txt", "b.txt", "c.txt"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_ok());
    CHECK(r.unwrap().get<std::string, 0>() == "a.txt");
}

TEST_CASE("Parser extra positional args error policy")
{
    App app("test", "1.0", "Strict error test");
    app.set_extra_args(ExtraArgsPolicy::Error);
    app.arg<std::string, 0>("file", "Input file");
    Argv argv{"test", "a.txt", "b.txt"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_err());
}

TEST_CASE("Parser extra positional args store policy")
{
    App app("test", "1.0", "Store test");
    app.set_extra_args(ExtraArgsPolicy::Store);
    app.arg<std::string, 0>("file", "Input file");
    Argv argv{"test", "a.txt", "b.txt", "c.txt"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_ok());
    auto &ctx = r.unwrap();
    CHECK(ctx.get<std::string, 0>() == "a.txt");
    auto extra = ctx.extra_args();
    CHECK(extra.size() == 2);
    CHECK(extra[0] == "b.txt");
    CHECK(extra[1] == "c.txt");
}

TEST_CASE("Parser extra positional args store no extra")
{
    App app("test", "1.0", "Store no extra");
    app.set_extra_args(ExtraArgsPolicy::Store);
    app.arg<std::string, 0>("file", "Input file");
    Argv argv{"test", "a.txt"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_ok());
    CHECK(r.unwrap().extra_args().empty());
}
