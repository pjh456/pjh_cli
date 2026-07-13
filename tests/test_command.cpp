#include <pjh_cli.hpp>
#include <doctest/doctest.h>
#include <string>
#include <string_view>

using namespace pjh::cli;

TEST_CASE("Command construction")
{
    App app("testapp", "1.0.0", "Test application");
    CHECK(app.name() == "testapp");
    CHECK(app.version() == "1.0.0");
    CHECK(app.description() == "Test application");
    CHECK(app.parent() == nullptr);
    CHECK(app.visibility() == Visibility::Both);
    CHECK(app.is_enabled());
}

TEST_CASE("Option bool flag no short no default")
{
    App app("test", "1.0", "Test");
    auto &opt = app.option<bool, fixed_string("verbose")>("--verbose", "Enable verbose output");
    CHECK(opt.long_name() == "verbose");
    CHECK(opt.short_name() == 0);
    CHECK(opt.has_value() == false);
    CHECK(opt.is_required() == false);
    CHECK(opt.key_hash() != 0);
}

TEST_CASE("Option int with short no default")
{
    App app("test", "1.0", "Test");
    auto &opt = app.option<int, fixed_string("port")>("--port", 'p', "Port number");
    CHECK(opt.long_name() == "port");
    CHECK(opt.short_name() == 'p');
    CHECK(opt.has_value() == true);
    CHECK(opt.key_hash() != 0);
}

TEST_CASE("Option string with short")
{
    App app("test", "1.0", "Test");
    auto &opt = app.option<std::string, fixed_string("host")>("--host", 'h', "Host address");
    CHECK(opt.long_name() == "host");
    CHECK(opt.short_name() == 'h');
    CHECK(opt.has_value() == true);
}

TEST_CASE("Option int no short")
{
    App app("test", "1.0", "Test");
    auto &opt = app.option<int, fixed_string("timeout")>("--timeout", "Timeout in seconds");
    CHECK(opt.long_name() == "timeout");
    CHECK(opt.short_name() == 0);
}

TEST_CASE("Option required chain")
{
    App app("test", "1.0", "Test");
    auto &opt = app.option<int, fixed_string("required-opt")>("--required-opt", 'r', "Required option").required();
    CHECK(opt.is_required() == true);
}

TEST_CASE("find_option_by_long")
{
    App app("test", "1.0", "Test");
    app.option<bool, fixed_string("verbose")>("--verbose", "Enable verbose output");
    app.option<int, fixed_string("port")>("--port", 'p', "Port number");

    auto *found1 = app.find_option_by_long("verbose");
    CHECK(found1 != nullptr);
    CHECK(found1->long_name() == "verbose");

    auto *found2 = app.find_option_by_long("port");
    CHECK(found2 != nullptr);
    CHECK(found2->long_name() == "port");

    auto *found3 = app.find_option_by_long("nonexistent");
    CHECK(found3 == nullptr);
}

TEST_CASE("find_option_by_short")
{
    App app("test", "1.0", "Test");
    app.option<int, fixed_string("port")>("--port", 'p', "Port number");

    auto *found4 = app.find_option_by_short('p');
    CHECK(found4 != nullptr);
    CHECK(found4->short_name() == 'p');

    auto *found5 = app.find_option_by_short('x');
    CHECK(found5 == nullptr);
}

TEST_CASE("Arg registration")
{
    App app("test", "1.0", "Test");

    auto &arg1 = app.arg<std::string, 0>("source", "Source file");
    CHECK(arg1.m_name == "source");
    CHECK(arg1.m_index == 0);
    CHECK(arg1.m_required == false);

    auto &arg2 = app.arg<std::string, 1>("dest", "Destination").required();
    CHECK(arg2.m_name == "dest");
    CHECK(arg2.m_index == 1);
    CHECK(arg2.m_required == true);
}

TEST_CASE("Subcommand tree basic")
{
    App app("test", "1.0", "Test");

    auto &serve = app.add_command("serve", "Start the server");
    CHECK(serve.name() == "serve");
    CHECK(serve.description() == "Start the server");
    CHECK(serve.parent() == &app);
    CHECK(serve.is_enabled());

    auto &start = serve.add_command("start", "Daemon subcommand");
    CHECK(start.name() == "start");
    CHECK(start.parent() == &serve);

    auto &stop = serve.add_command("stop", "Stop the server");
    CHECK(stop.name() == "stop");

    CHECK(serve.subcommands().size() == 2);
}

TEST_CASE("find_subcommand")
{
    App app("test", "1.0", "Test");
    auto &serve = app.add_command("serve", "Start the server");
    serve.add_command("start", "Daemon subcommand");
    serve.add_command("stop", "Stop the server");

    auto *found_cmd = app.find_subcommand("serve");
    CHECK(found_cmd != nullptr);
    CHECK(found_cmd->name() == "serve");

    auto *not_found = app.find_subcommand("nonexistent");
    CHECK(not_found == nullptr);

    auto *inner = serve.find_subcommand("start");
    CHECK(inner != nullptr);
    CHECK(inner->name() == "start");
}

TEST_CASE("subcommand visibility")
{
    App app("test", "1.0", "Test");
    auto &hidden_cmd = app.add_command("debug", "Debug command");
    hidden_cmd.set_visibility(Visibility::Hidden);
    CHECK(hidden_cmd.visibility() == Visibility::Hidden);
}

TEST_CASE("subcommand enabled predicate")
{
    App app("test", "1.0", "Test");

    bool flag = false;
    auto &cond_cmd = app.add_command("conditional", "Conditional");
    cond_cmd.enabled([&flag] { return flag; });
    CHECK(!cond_cmd.is_enabled());
    flag = true;
    CHECK(cond_cmd.is_enabled());
}

TEST_CASE("subcommand action callback")
{
    App app("test", "1.0", "Test");

    int action_called = 0;
    auto &act_cmd = app.add_command("act", "Action test");
    act_cmd.action([&action_called](ParseContext &) -> CliResult<void> {
        ++action_called;
        return CliResult<void>::Ok();
    });
}

TEST_CASE("ParseContext")
{
    App app("test", "1.0", "Test");

    ParseContext ctx;
    ctx.set_value<int>(key_hash(fixed_string("port")), 8080);
    ctx.set_value<std::string>(key_hash(fixed_string("host")), std::string("localhost"));
    ctx.set_value<unsigned>(key_hash(static_cast<size_t>(0)), 42u);

    auto port = ctx.get<int, fixed_string("port")>();
    CHECK(port == 8080);

    auto &host = ctx.get<std::string, fixed_string("host")>();
    CHECK(host == "localhost");

    auto idx0 = ctx.get<unsigned, 0>();
    CHECK(idx0 == 42u);

    CHECK(ctx.has<fixed_string("port")>());
    CHECK(!ctx.has<fixed_string("nonexistent")>());
    CHECK(ctx.has<0>());
    CHECK(!ctx.has<999>());

    CHECK(ctx.has_value(key_hash(fixed_string("port"))));
    CHECK(!ctx.has_value(key_hash(fixed_string("missing"))));

    {
        auto some_port = ctx.try_get<int, fixed_string("port")>();
        CHECK(some_port.is_some());
        CHECK(some_port.unwrap() == 8080);

        auto none_port = ctx.try_get<int, fixed_string("missing")>();
        CHECK(none_port.is_none());

        auto some_idx0 = ctx.try_get<unsigned, 0>();
        CHECK(some_idx0.is_some());
        CHECK(some_idx0.unwrap() == 42u);

        auto none_idx = ctx.try_get<unsigned, 999>();
        CHECK(none_idx.is_none());
    }

    ctx.set_matched_path("serve start");
    CHECK(ctx.matched_path() == "serve start");

    auto cmd_ctx = app.create_context();
    CHECK(!cmd_ctx.has<fixed_string("port")>());
}

TEST_CASE("Command apply defaults")
{
    App app2("test2", "1.0", "Default test");
    app2.option<int, fixed_string("x")>("--x", "X value", 100);
    auto ctx2 = app2.create_context();
    auto res = app2.apply_defaults(ctx2);
    CHECK(res.is_ok());
    CHECK(ctx2.has<fixed_string("x")>());

    int xval = ctx2.get<int, fixed_string("x")>();
    CHECK(xval == 100);

    ctx2.set_value<int>(key_hash(fixed_string("x")), 200);
    int xval2 = ctx2.get<int, fixed_string("x")>();
    CHECK(xval2 == 200);
}

TEST_CASE("Command execute")
{
    App app3("test3", "1.0", "Execute test");
    int counter = 0;
    app3.action([&counter](ParseContext &) -> CliResult<void> {
        ++counter;
        return CliResult<void>::Ok();
    });

    auto ctx3 = app3.create_context();
    auto res = app3.execute(ctx3);
    CHECK(res.is_ok());
    CHECK(counter == 1);

    auto &noact = app3.add_command("noact", "No action");
    auto ctx4 = noact.create_context();
    res = noact.execute(ctx4);
    CHECK(res.is_ok());
}

TEST_CASE("Command options count")
{
    App app("test", "1.0", "Test");
    app.option<bool, fixed_string("verbose")>("--verbose", "Enable verbose output");
    app.option<int, fixed_string("port")>("--port", 'p', "Port number");
    app.option<std::string, fixed_string("host")>("--host", 'h', "Host address", std::string("0.0.0.0"));
    app.option<int, fixed_string("timeout")>("--timeout", "Timeout in seconds", 30);
    app.option<int, fixed_string("required-opt")>("--required-opt", 'r', "Required option").required();
    app.arg<std::string, 0>("source", "Source file");
    app.arg<std::string, 1>("dest", "Destination").required();

    CHECK(app.options().size() == 5);
    CHECK(app.args().size() == 2);
}

TEST_CASE("key_hash constexpr correctness")
{
    constexpr auto h1 = key_hash(fixed_string("test"));
    constexpr auto h2 = key_hash(fixed_string("test"));
    constexpr auto h3 = key_hash(size_t{0});
    constexpr auto h4 = key_hash(size_t{42});
    static_assert(h1 == h2);
    static_assert(h1 != h3);
    static_assert(h3 == 0);
    static_assert(h4 == 42);
}
