#include <doctest/doctest.h>

#include <cstddef>
#include <format>
#include <iostream>
#include <memory>
#include <pjh_cli/app.hpp>
#include <pjh_cli/command/base_command.hpp>
#include <pjh_cli/command/leaf_command.hpp>
#include <pjh_cli/core/fixed_string.hpp>
#include <pjh_cli/core/type.hpp>
#include <pjh_cli/option/option_def.hpp>
#include <pjh_cli/parse/detail/parse_context_writer.hpp>
#include <pjh_cli/parse/matched_path_resolver.hpp>
#include <pjh_cli/parse/parse_context.hpp>
#include <pjh_cli/parse/parse_finalizer.hpp>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

using namespace pjh::cli;

// Contract pin: subcommands() is a read-only view.  A mutable return type would
// let callers insert/erase/reorder children and desync parent()/find_subcommand().
static_assert(
    std::is_const_v<
        std::remove_reference_t<decltype(std::declval<BranchCommand &>().subcommands())>>,
    "BranchCommand::subcommands() must return a const reference");

// EnvSnapshot must not appear in the public command API.
template <typename T>
concept HasEnvSnapshotAccessor = requires(T &t) { t.env_snapshot(); };
static_assert(!HasEnvSnapshotAccessor<BaseCommand>);
static_assert(!HasEnvSnapshotAccessor<App>);

// The replacement const interface is present and type-free.
static_assert(std::is_same_v<
              decltype(std::declval<const App &>().env_value(std::string_view{})),
              const std::string *>);

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
    auto &opt = app.option<fixed_string("verbose")>("--verbose", "Enable verbose output")
                    .boolean();
    CHECK(opt.long_name() == "verbose");
    CHECK(opt.short_name() == 0);
    CHECK(opt.has_value() == false);
    CHECK(opt.is_required() == false);
    CHECK(opt.key_hash() != 0);
}

TEST_CASE("Option display_name returns canonical long form")
{
    App app("test", "1.0", "Test");
    auto &opt = app.option<fixed_string("verbose")>("--verbose", "Enable verbose output")
                    .boolean();
    CHECK(opt.display_name() == "--verbose");
}

TEST_CASE("Option int with short no default")
{
    App app("test", "1.0", "Test");
    auto &opt = app.option<fixed_string("port")>("--port", 'p', "Port number").integer();
    CHECK(opt.long_name() == "port");
    CHECK(opt.short_name() == 'p');
    CHECK(opt.has_value() == true);
    CHECK(opt.key_hash() != 0);
}

TEST_CASE("Option string with short")
{
    App app("test", "1.0", "Test");
    auto &opt = app.option<fixed_string("host")>("--host", 'H', "Host address").str();
    CHECK(opt.long_name() == "host");
    CHECK(opt.short_name() == 'H');
    CHECK(opt.has_value() == true);
}

TEST_CASE("Option int no short")
{
    App app("test", "1.0", "Test");
    auto &opt =
        app.option<fixed_string("timeout")>("--timeout", "Timeout in seconds").integer();
    CHECK(opt.long_name() == "timeout");
    CHECK(opt.short_name() == 0);
}

TEST_CASE("Option required chain")
{
    App app("test", "1.0", "Test");
    auto &opt =
        app.option<fixed_string("required-opt")>("--required-opt", 'r', "Required option")
            .integer()
            .required();
    CHECK(opt.is_required() == true);
}

TEST_CASE("find_option_by_long")
{
    App app("test", "1.0", "Test");
    app.option<fixed_string("verbose")>("--verbose", "Enable verbose output").boolean();
    app.option<fixed_string("port")>("--port", 'p', "Port number").integer();

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
    app.option<fixed_string("port")>("--port", 'p', "Port number").integer();

    auto *found4 = app.find_option_by_short('p');
    CHECK(found4 != nullptr);
    CHECK(found4->short_name() == 'p');

    auto *found5 = app.find_option_by_short('x');
    CHECK(found5 == nullptr);
}

TEST_CASE("Arg registration")
{
    LeafCommand cmd("test", "Test");

    auto &arg1 = cmd.arg<std::string, 0>("source", "Source file");
    CHECK(arg1.m_name == "source");
    CHECK(arg1.m_key_hash == 0);
    CHECK(arg1.m_required == false);

    auto &arg2 = cmd.arg<std::string, 1>("dest", "Destination").required();
    CHECK(arg2.m_name == "dest");
    CHECK(arg2.m_key_hash == 1);
    CHECK(arg2.m_required == true);
}

TEST_CASE("Subcommand tree basic")
{
    App app("test", "1.0", "Test");

    auto &serve = app.add_branch("serve", "Start the server");
    CHECK(serve.name() == "serve");
    CHECK(serve.description() == "Start the server");
    CHECK(serve.parent() == &app);
    CHECK(serve.is_enabled());

    auto &start = serve.add_branch("start", "Daemon subcommand");
    CHECK(start.name() == "start");
    CHECK(start.parent() == &serve);

    auto &stop = serve.add_branch("stop", "Stop the server");
    CHECK(stop.name() == "stop");

    CHECK(serve.subcommands().size() == 2);
}

TEST_CASE("subcommands() is a read-only view with intact parent and name index")
{
    App app("test", "1.0", "Read-only view");
    auto &serve = app.add_branch("serve", "Serve");
    auto &start = serve.add_branch("start", "Start");
    auto &stop = serve.add_leaf("stop", "Stop");

    const BranchCommand &view = serve;
    CHECK(view.subcommands().size() == 2);
    CHECK(view.subcommands()[0]->name() == "start");  // registration order
    CHECK(view.subcommands()[1]->name() == "stop");
    for (const auto &child : view.subcommands())
    {
        CHECK(child->parent() == &serve);
        CHECK(serve.find_subcommand(child->name()) == child.get());
    }
    CHECK(start.parent() == &serve);
    CHECK(stop.parent() == &serve);
}

TEST_CASE("find_subcommand")
{
    App app("test", "1.0", "Test");
    auto &serve = app.add_branch("serve", "Start the server");
    serve.add_branch("start", "Daemon subcommand");
    serve.add_branch("stop", "Stop the server");

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
    auto &hidden_cmd = app.add_branch("debug", "Debug command");
    hidden_cmd.set_visibility(Visibility::Hidden);
    CHECK(hidden_cmd.visibility() == Visibility::Hidden);
}

TEST_CASE("subcommand enabled predicate")
{
    App app("test", "1.0", "Test");

    bool flag = false;
    auto &cond_cmd = app.add_branch("conditional", "Conditional");
    cond_cmd.enabled([&flag] { return flag; });
    CHECK(!cond_cmd.is_enabled());
    flag = true;
    CHECK(cond_cmd.is_enabled());
}

TEST_CASE("subcommand action callback")
{
    App app("test", "1.0", "Test");

    int action_called = 0;
    auto &act_cmd = app.add_leaf("act", "Action test");
    act_cmd.action(
        [&action_called](ParseContext &) -> CliResult<void>
        {
            ++action_called;
            return CliResult<void>::Ok();
        });
}

TEST_CASE("ParseContext")
{
    App app("test", "1.0", "Test");

    ParseContext ctx;
    detail::ParseContextWriter::set_value<int>(ctx, key_hash(fixed_string("port")), 8080);
    detail::ParseContextWriter::set_value<std::string>(
        ctx, key_hash(fixed_string("host")), std::string("localhost"));
    detail::ParseContextWriter::set_value<int>(ctx, key_hash(static_cast<size_t>(0)), 42);

    auto port = ctx.get<int, fixed_string("port")>();
    CHECK(port == 8080);

    auto &host = ctx.get<std::string, fixed_string("host")>();
    CHECK(host == "localhost");

    auto idx0 = ctx.get<int, 0>();
    CHECK(idx0 == 42);

    CHECK(ctx.has<fixed_string("port")>());
    CHECK(!ctx.has<fixed_string("nonexistent")>());
    CHECK(ctx.has<0>());
    CHECK(!ctx.has<999>());

    CHECK(detail::ParseContextWriter::has_value(ctx, key_hash(fixed_string("port"))));
    CHECK(!detail::ParseContextWriter::has_value(ctx, key_hash(fixed_string("missing"))));

    detail::ParseContextWriter::set_matched_command(ctx, &app);
    CHECK(MatchedPathResolver::to_path_string(ctx.matched_command()) == "test");

    ParseContext cmd_ctx;
    CHECK(!cmd_ctx.has<fixed_string("port")>());
}

TEST_CASE("Command apply defaults")
{
    App app2("test2", "1.0", "Default test");
    app2.option<fixed_string("x")>("--x", "X value", 100);
    ParseContext ctx2;
    auto res = ParseFinalizer::apply_defaults(app2, ctx2);
    CHECK(res.is_ok());
    CHECK(ctx2.has<fixed_string("x")>());

    int xval = ctx2.get<int, fixed_string("x")>();
    CHECK(xval == 100);

    detail::ParseContextWriter::set_value<int>(ctx2, key_hash(fixed_string("x")), 200);
    int xval2 = ctx2.get<int, fixed_string("x")>();
    CHECK(xval2 == 200);
}

TEST_CASE("Command execute")
{
    App app3("test3", "1.0", "Execute test");
    int counter = 0;
    app3.action(
        [&counter](ParseContext &) -> CliResult<void>
        {
            ++counter;
            return CliResult<void>::Ok();
        });

    ParseContext ctx3;
    auto res = app3.execute(ctx3);
    CHECK(res.is_ok());
    CHECK(counter == 1);

    auto &noact = app3.add_leaf("noact", "No action");
    ParseContext ctx4;
    res = noact.execute(ctx4);
    CHECK(res.is_ok());
}

TEST_CASE("ParseFinalizer apply_defaults skips present values")
{
    App app("test", "1.0", "Skip present");
    app.option<fixed_string("x")>("--x", "X", 100);
    ParseContext ctx;
    detail::ParseContextWriter::set_value<int>(ctx, key_hash(fixed_string("x")), 200);
    auto res = ParseFinalizer::apply_defaults(app, ctx);
    CHECK(res.is_ok());
    CHECK(ctx.get<int, fixed_string("x")>() == 200);
}

TEST_CASE("Command options count")
{
    LeafCommand cmd("test", "Test");
    cmd.option<fixed_string("verbose")>("--verbose", "Enable verbose output").boolean();
    cmd.option<fixed_string("port")>("--port", 'p', "Port number").integer();
    cmd.option<fixed_string("host")>(
        "--host", 'H', "Host address", std::string("0.0.0.0"));
    cmd.option<fixed_string("timeout")>("--timeout", "Timeout in seconds", 30);
    cmd.option<fixed_string("required-opt")>("--required-opt", 'r', "Required option")
        .integer()
        .required();
    cmd.arg<std::string, 0>("source", "Source file");
    cmd.arg<std::string, 1>("dest", "Destination").required();

    CHECK(cmd.options().size() == 5);
    CHECK(cmd.args().size() == 2);
}

TEST_CASE("OptionDef reference stability across registrations")
{
    App app("test", "1.0", "Test");

    auto &first = app.option<0>("--opt0", "original").integer();
    auto *addr = &first;

    [&app]<size_t... Is>(std::index_sequence<Is...>)
    {
        ((void)app.option<Is + 1>(std::format("--opt{}", Is + 1), "").integer(), ...);
    }(std::make_index_sequence<30>());

    CHECK(&first == addr);
    CHECK(first.description() == "original");
    CHECK(first.long_name() == "opt0");
    first.set_description("modified");
    CHECK(first.description() == "modified");
}

TEST_CASE("ArgDef reference stability across registrations")
{
    LeafCommand cmd("test", "Test");

    auto &first = cmd.arg<std::string, 0>("arg0", "original");
    auto *addr = &first;

    [&cmd]<size_t... Is>(std::index_sequence<Is...>)
    {
        ((void)cmd.arg<std::string, Is + 1>(std::format("arg{}", Is + 1), ""), ...);
    }(std::make_index_sequence<30>());

    CHECK(&first == addr);
    CHECK(first.m_name == "arg0");
    CHECK(first.m_key_hash == 0);
    first.m_name = "modified";
    CHECK(first.m_name == "modified");
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

TEST_CASE("OptionDef completer callback stored and callable")
{
    App app("test", "1.0", "Completer");
    app.option<fixed_string("color")>("--color", "Color")
        .str()
        .completer([]() -> std::vector<std::string> { return {"red", "green", "blue"}; });

    auto *def = app.find_option_by_long("color");
    REQUIRE(def != nullptr);
    auto &fn = def->completer_fn();
    REQUIRE(fn);
    auto candidates = fn();
    REQUIRE(candidates.size() == 3);
    CHECK(candidates[0] == "red");
    CHECK(candidates[1] == "green");
    CHECK(candidates[2] == "blue");
}

TEST_CASE("OptionDef completer empty when not registered")
{
    App app("test", "1.0", "No completer");
    app.option<fixed_string("port")>("--port", "Port").integer();

    auto *def = app.find_option_by_long("port");
    REQUIRE(def != nullptr);
    CHECK_FALSE(def->completer_fn());
}

TEST_CASE("reserved long option --help is rejected at registration")
{
    App app("test", "1.0", "Reserved help");
    CHECK_THROWS_WITH_AS(
        app.option<fixed_string("help")>("--help", "Help").boolean(),
        "BaseCommand::add_option: option '--help' is reserved for the built-in "
        "--help/-h/--version flags on command 'test'",
        LogicError);
    CHECK(app.options().empty());
    CHECK(app.find_option_by_long("help") == nullptr);
}

TEST_CASE("reserved long option --version is rejected at registration")
{
    App app("test", "1.0", "Reserved version");
    CHECK_THROWS_WITH_AS(
        app.option<fixed_string("version")>("--version", "Version").boolean(),
        "BaseCommand::add_option: option '--version' is reserved for the built-in "
        "--help/-h/--version flags on command 'test'",
        LogicError);
    CHECK(app.options().empty());
}

TEST_CASE("reserved short option -h is rejected at registration")
{
    App app("test", "1.0", "Reserved short");
    CHECK_THROWS_WITH_AS(
        app.option<fixed_string("host")>("--host", 'h', "Host").str(),
        "BaseCommand::add_option: option '-h' is reserved for the built-in "
        "--help/-h/--version flags on command 'test'",
        LogicError);
    CHECK(app.options().empty());
    CHECK(app.find_option_by_long("host") == nullptr);  // no partial long insert
    CHECK(app.find_option_by_short('h') == nullptr);
}

TEST_CASE("reserved names are rejected without the -- prefix")
{
    App app("test", "1.0", "Reserved normalized");
    CHECK_THROWS_AS(
        app.option<fixed_string("help")>("help", "Help").boolean(), LogicError);
    CHECK_THROWS_AS(
        app.option<fixed_string("version")>("version", "V").boolean(), LogicError);
}

TEST_CASE("near-reserved option names are allowed")
{
    App app("test", "1.0", "Near reserved");
    CHECK_NOTHROW(app.option<fixed_string("helper")>("--helper", "Helper").boolean());
    CHECK_NOTHROW(
        app.option<fixed_string("versioned")>("--versioned", "Versioned").boolean());
    CHECK_NOTHROW(app.option<fixed_string("host")>("--host", 'H', "Host").str());
    CHECK(app.options().size() == 3);
    CHECK(app.find_option_by_short('H') != nullptr);
}

TEST_CASE("reserved name rejection is case-sensitive")
{
    App app("test", "1.0", "Case");
    CHECK_NOTHROW(app.option<fixed_string("h")>("--Help", "Help upper").boolean());
    CHECK_NOTHROW(app.option<fixed_string("v")>("--Version", "Version upper").boolean());
    CHECK(app.options().size() == 2);
}

TEST_CASE("reserved names are rejected on non-root commands")
{
    App app("test", "1.0", "Reserved child");
    auto &sub = app.add_leaf("sub", "Sub");
    CHECK_THROWS_AS(
        sub.option<fixed_string("help")>("--help", "Help").boolean(), LogicError);
    CHECK_THROWS_AS(
        sub.option<fixed_string("host")>("--host", 'h', "Host").str(), LogicError);
    CHECK(sub.options().empty());
    CHECK(app.options().empty());
}

TEST_CASE("reserved-name rejection leaves indexes and ownership untouched")
{
    App app("test", "1.0", "Atomic");
    app.option<fixed_string("ok")>("--ok", 'o', "OK").boolean();

    CHECK_THROWS_AS(
        app.option<fixed_string("host")>("--host", 'h', "Host").str(), LogicError);
    CHECK(app.options().size() == 1);
    CHECK(app.find_option_by_long("host") == nullptr);
    CHECK(app.find_option_by_short('h') == nullptr);
    CHECK(app.find_option_by_short('o') != nullptr);
}

TEST_CASE("duplicate long option name on one command throws LogicError")
{
    App app("test", "1.0", "Dup long");
    auto &first = app.option<fixed_string("foo1")>("--foo", 'z', "Foo").boolean();
    CHECK_THROWS_WITH_AS(
        app.option<fixed_string("foo2")>("--foo", "Foo2").boolean(),
        "BaseCommand::add_option: duplicate long option '--foo' on command 'test'",
        LogicError);
    CHECK(app.options().size() == 1);
    CHECK(app.find_option_by_long("foo") == &first);
    CHECK(app.find_option_by_short('z') == &first);
}

TEST_CASE("duplicate short option name on one command throws LogicError")
{
    App app("test", "1.0", "Dup short");
    auto &first = app.option<fixed_string("foo")>("--foo", 'f', "Foo").boolean();
    CHECK_THROWS_WITH_AS(
        app.option<fixed_string("bar")>("--bar", 'f', "Bar").boolean(),
        "BaseCommand::add_option: duplicate short option '-f' on command 'test'",
        LogicError);
    CHECK(app.options().size() == 1);
    CHECK(app.find_option_by_long("bar") == nullptr);
    CHECK(app.find_option_by_short('f') == &first);
}

TEST_CASE("duplicate long option name is detected after -- normalization")
{
    App app("test", "1.0", "Normalized");
    app.option<fixed_string("foo1")>("--foo", "Foo").boolean();
    CHECK_THROWS_AS(
        app.option<fixed_string("foo2")>("foo", "Foo2").boolean(), LogicError);
    CHECK(app.options().size() == 1);
}

TEST_CASE("the same long option name is allowed on ancestor and child commands")
{
    App app("test", "1.0", "Cross long");
    auto &root_opt = app.option<fixed_string("root_opt")>("--opt", "Root").boolean();
    auto &son = app.add_leaf("son", "Son");
    auto &child_opt = son.option<fixed_string("child_opt")>("--opt", "Child").boolean();
    CHECK(app.options().size() == 1);
    CHECK(son.options().size() == 1);
    CHECK(app.find_option_by_long("opt") == &root_opt);
    CHECK(son.find_option_by_long("opt") == &child_opt);
}

TEST_CASE("the same short option name is allowed on ancestor and child commands")
{
    App app("test", "1.0", "Cross short");
    auto &root_opt =
        app.option<fixed_string("root_opt")>("--root", 'r', "Root").boolean();
    auto &son = app.add_leaf("son", "Son");
    auto &child_opt =
        son.option<fixed_string("child_opt")>("--child", 'r', "Child").boolean();
    CHECK(app.find_option_by_short('r') == &root_opt);
    CHECK(son.find_option_by_short('r') == &child_opt);
}

TEST_CASE("short-less options do not collide on the zero sentinel")
{
    App app("test", "1.0", "No short");
    CHECK_NOTHROW(app.option<fixed_string("a")>("--alpha", "Alpha").boolean());
    CHECK_NOTHROW(app.option<fixed_string("b")>("--beta", "Beta").boolean());
    CHECK(app.options().size() == 2);
    CHECK(app.find_option_by_short('\0') == nullptr);
}

TEST_CASE("duplicate option name checks are case-sensitive")
{
    App app("test", "1.0", "Case");
    CHECK_NOTHROW(app.option<fixed_string("lower")>("--foo", 'f', "Lower").boolean());
    CHECK_NOTHROW(app.option<fixed_string("upper")>("--Foo", 'F', "Upper").boolean());
    CHECK(app.options().size() == 2);
    CHECK(app.find_option_by_long("foo") != nullptr);
    CHECK(app.find_option_by_long("Foo") != nullptr);
}

TEST_CASE("direct add_option rejects a duplicate long name")
{
    App app("test", "1.0", "Direct");
    app.option<fixed_string("foo")>("--foo", 'f', "Foo").boolean();

    auto second = std::make_unique<OptionDef>();
    second->set_long_name("foo");
    second->set_short_name('g');
    CHECK_THROWS_AS(app.add_option(std::move(second)), LogicError);
    CHECK(app.options().size() == 1);
    CHECK(app.find_option_by_short('g') == nullptr);
}

TEST_CASE("an explicit --no- option is distinct from a negatable option")
{
    App app("test", "1.0", "Negation");
    CHECK_NOTHROW(app.option<fixed_string("foo")>("--foo", "Foo").boolean().negatable());
    CHECK_NOTHROW(
        app.option<fixed_string("no_foo")>("--no-foo", 'n', "Explicit").boolean());
    CHECK(app.options().size() == 2);
    CHECK(app.find_option_by_long("no-foo") != nullptr);
}

TEST_CASE("a rejected duplicate leaves the command usable")
{
    App app("test", "1.0", "Recover");
    app.option<fixed_string("foo")>("--foo", 'f', "Foo").boolean();
    CHECK_THROWS_AS(
        app.option<fixed_string("foo2")>("--foo", 'g', "Dup").boolean(), LogicError);

    CHECK_NOTHROW(app.option<fixed_string("bar")>("--bar", 'b', "Bar").integer());
    CHECK(app.options().size() == 2);
    CHECK(app.find_option_by_long("bar") != nullptr);
    CHECK(app.find_option_by_short('b') != nullptr);
    CHECK(app.find_option_by_long("foo") != nullptr);
}
