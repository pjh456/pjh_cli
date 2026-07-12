#include <pjh_cli.hpp>
#include <cassert>
#include <string>
#include <string_view>

using namespace pjh::cli;

int main()
{
    // ── App construction ──

    App app("testapp", "1.0.0", "Test application");
    assert(app.name() == "testapp");
    assert(app.version() == "1.0.0");
    assert(app.description() == "Test application");
    assert(app.parent() == nullptr);
    assert(app.visibility() == Visibility::Both);
    assert(app.is_enabled());

    // ── Option registration (no short name, no default) ──

    auto &opt1 = app.option<bool, fixed_string("verbose")>(
        "--verbose", "Enable verbose output");
    assert(opt1.m_long_name == "verbose");
    assert(opt1.m_short_name == 0);
    assert(opt1.m_has_value == false); // bool = flag
    assert(opt1.m_required == false);
    assert(opt1.m_key_hash != 0);

    // ── Option registration (short name, no default) ──

    auto &opt2 = app.option<int, fixed_string("port")>(
        "--port", 'p', "Port number");
    assert(opt2.m_long_name == "port");
    assert(opt2.m_short_name == 'p');
    assert(opt2.m_has_value == true); // int = valued
    assert(opt2.m_key_hash != 0);

    // ── Option registration (short name, with default) ──

    auto &opt3 = app.option<std::string, fixed_string("host")>(
        "--host", 'h', "Host address", std::string("0.0.0.0"));
    assert(opt3.m_long_name == "host");
    assert(opt3.m_short_name == 'h');
    assert(opt3.m_has_value == true);
    assert(static_cast<bool>(opt3.m_apply_default));

    // ── Option registration (no short name, with default) ──

    auto &opt4 = app.option<int, fixed_string("timeout")>(
        "--timeout", "Timeout in seconds", 30);
    assert(opt4.m_long_name == "timeout");
    assert(opt4.m_short_name == 0);
    assert(static_cast<bool>(opt4.m_apply_default));

    // ── Option required chain ──

    auto &opt5 = app.option<int, fixed_string("required-opt")>(
                        "--required-opt", 'r', "Required option")
                     .required();
    assert(opt5.m_required == true);

    // ── Option lookup ──

    auto *found1 = app.find_option_by_long("verbose");
    assert(found1 != nullptr);
    assert(found1->m_long_name == "verbose");

    auto *found2 = app.find_option_by_long("port");
    assert(found2 != nullptr);
    assert(found2->m_long_name == "port");

    auto *found3 = app.find_option_by_long("nonexistent");
    assert(found3 == nullptr);

    auto *found4 = app.find_option_by_short('p');
    assert(found4 != nullptr);
    assert(found4->m_short_name == 'p');

    auto *found5 = app.find_option_by_short('x');
    assert(found5 == nullptr);

    // ── Arg registration ──

    auto &arg1 = app.arg<std::string, 0>("source", "Source file");
    assert(arg1.m_name == "source");
    assert(arg1.m_index == 0);
    assert(arg1.m_required == false);

    auto &arg2 = app.arg<std::string, 1>("dest", "Destination").required();
    assert(arg2.m_name == "dest");
    assert(arg2.m_index == 1);
    assert(arg2.m_required == true);

    // ── Subcommand tree ──

    auto &serve = app.add_command("serve", "Start the server");
    assert(serve.name() == "serve");
    assert(serve.description() == "Start the server");
    assert(serve.parent() == &app);
    assert(serve.is_enabled());

    auto &start = serve.add_command("start", "Daemon subcommand");
    assert(start.name() == "start");
    assert(start.parent() == &serve);

    auto &stop = serve.add_command("stop", "Stop the server");
    assert(stop.name() == "stop");

    // find_subcommand
    auto *found_cmd = app.find_subcommand("serve");
    assert(found_cmd != nullptr);
    assert(found_cmd->name() == "serve");

    auto *not_found = app.find_subcommand("nonexistent");
    assert(not_found == nullptr);

    auto *inner = serve.find_subcommand("start");
    assert(inner != nullptr);
    assert(inner->name() == "start");

    // subcommands list
    auto &children = serve.subcommands();
    assert(children.size() == 2);

    // ── Visibility ──

    auto &hidden_cmd = app.add_command("debug", "Debug command");
    hidden_cmd.set_visibility(Visibility::Hidden);
    assert(hidden_cmd.visibility() == Visibility::Hidden);

    // ── Enabled predicate ──

    bool flag = false;
    auto &cond_cmd = app.add_command("conditional", "Conditional");
    cond_cmd.enabled([&flag]
                     { return flag; });
    assert(!cond_cmd.is_enabled());
    flag = true;
    assert(cond_cmd.is_enabled());

    // ── Action callback ──

    int action_called = 0;
    auto &act_cmd = app.add_command("act", "Action test");
    act_cmd.action([&action_called](ParseContext &) -> ParseResult<void>
                   {
        ++action_called;
        return ParseResult<void>::Ok(); });

    // ── ParseContext: basic get/set ──

    ParseContext ctx;
    ctx.set_value<int>(key_hash(fixed_string("port")), 8080);
    ctx.set_value<std::string>(
        key_hash(fixed_string("host")), std::string("localhost"));
    ctx.set_value<unsigned>(key_hash(static_cast<size_t>(0)), 42u);

    auto port = ctx.get<int, fixed_string("port")>();
    assert(port == 8080);

    auto &host = ctx.get<std::string, fixed_string("host")>();
    assert(host == "localhost");

    auto idx0 = ctx.get<unsigned, 0>();
    assert(idx0 == 42u);

    // ParseContext::has
    assert(ctx.has<fixed_string("port")>());
    assert(!ctx.has<fixed_string("nonexistent")>());
    assert(ctx.has<0>());
    assert(!ctx.has<999>());

    // ParseContext::has_value (runtime)
    assert(ctx.has_value(key_hash(fixed_string("port"))));
    assert(!ctx.has_value(key_hash(fixed_string("missing"))));

    // ParseContext::try_get — Some/None
    {
        auto some_port = ctx.try_get<int, fixed_string("port")>();
        assert(some_port.is_some());
        assert(some_port.unwrap() == 8080);

        auto none_port = ctx.try_get<int, fixed_string("missing")>();
        assert(none_port.is_none());

        auto some_idx0 = ctx.try_get<unsigned, 0>();
        assert(some_idx0.is_some());
        assert(some_idx0.unwrap() == 42u);

        auto none_idx = ctx.try_get<unsigned, 999>();
        assert(none_idx.is_none());
    }

    // ParseContext: matched_path
    ctx.set_matched_path("serve start");
    assert(ctx.matched_path() == "serve start");

    // ── Command::create_context ──

    auto cmd_ctx = app.create_context();
    assert(!cmd_ctx.has<fixed_string("port")>());

    // ── Command::apply_defaults ──

    {
        App app2("test2", "1.0", "Default test");
        app2.option<int, fixed_string("x")>("--x", "X value", 100);
        auto ctx2 = app2.create_context();
        auto res = app2.apply_defaults(ctx2);
        assert(res.is_ok());
        assert(ctx2.has<fixed_string("x")>());

        int xval = ctx2.get<int, fixed_string("x")>();
        assert(xval == 100);

        // Override in ParseContext should win
        ctx2.set_value<int>(key_hash(fixed_string("x")), 200);
        int xval2 = ctx2.get<int, fixed_string("x")>();
        assert(xval2 == 200);
    }

    // ── Command::execute ──

    {
        App app3("test3", "1.0", "Execute test");
        int counter = 0;
        app3.action([&counter](ParseContext &) -> ParseResult<void>
                    {
            ++counter;
            return ParseResult<void>::Ok(); });

        auto ctx3 = app3.create_context();
        auto res = app3.execute(ctx3);
        assert(res.is_ok());
        assert(counter == 1);

        // No action -> no-op, not an error
        auto &noact = app3.add_command("noact", "No action");
        auto ctx4 = noact.create_context();
        res = noact.execute(ctx4);
        assert(res.is_ok());
    }

    // ── Options list iteration ──

    assert(app.options().size() == 5); // verbose, port, host, timeout, required-opt
    assert(app.args().size() == 2);    // source, dest

    // ── key_hash constexpr correctness ──

    constexpr auto h1 = key_hash(fixed_string("test"));
    constexpr auto h2 = key_hash(fixed_string("test"));
    constexpr auto h3 = key_hash(size_t{0});
    constexpr auto h4 = key_hash(size_t{42});
    static_assert(h1 == h2);
    static_assert(h1 != h3);
    static_assert(h3 == 0);
    static_assert(h4 == 42);

    // ── Converter is used correctly in option apply lambdas ──
    // (indirectly tested: if Converter<T> were not available,
    //  the option templates wouldn't compile)

    return 0;
}
