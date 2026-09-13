#include <doctest/doctest.h>

#include <iostream>
#include <pjh_cli/app.hpp>
#include <pjh_cli/command/base_command.hpp>
#include <pjh_cli/command/matcher.hpp>
#include <pjh_cli/option/option_def.hpp>
#include <string>
#include <string_view>
#include <utility>

using namespace pjh::cli;
using namespace pjh::cli::detail;

static_assert(
    !noexcept(
        is_visible_and_enabled(std::declval<const BaseCommand &>(), Visibility::Both)),
    "is_visible_and_enabled invokes the user enabled predicate");

TEST_CASE("is_visible_and_enabled both modes")
{
    App cmd("test", "1.0", "");
    CHECK(is_visible_and_enabled(cmd, Visibility::Both) == true);
    CHECK(is_visible_and_enabled(cmd, Visibility::Repl) == true);
    CHECK(is_visible_and_enabled(cmd, Visibility::Cli) == true);
}

TEST_CASE("is_visible_and_enabled hidden")
{
    App cmd("test", "1.0", "");
    cmd.set_visibility(Visibility::Hidden);
    CHECK(is_visible_and_enabled(cmd, Visibility::Both) == false);
    CHECK(is_visible_and_enabled(cmd, Visibility::Repl) == false);
    CHECK(is_visible_and_enabled(cmd, Visibility::Cli) == false);
}

TEST_CASE("is_visible_and_enabled repl only")
{
    App cmd("test", "1.0", "");
    cmd.set_visibility(Visibility::Repl);
    CHECK(is_visible_and_enabled(cmd, Visibility::Both) == true);
    CHECK(is_visible_and_enabled(cmd, Visibility::Repl) == true);
    CHECK(is_visible_and_enabled(cmd, Visibility::Cli) == false);
}

TEST_CASE("is_visible_and_enabled cli only")
{
    App cmd("test", "1.0", "");
    cmd.set_visibility(Visibility::Cli);
    CHECK(is_visible_and_enabled(cmd, Visibility::Both) == true);
    CHECK(is_visible_and_enabled(cmd, Visibility::Repl) == false);
    CHECK(is_visible_and_enabled(cmd, Visibility::Cli) == true);
}

TEST_CASE("is_visible_and_enabled disabled")
{
    App cmd("test", "1.0", "");
    cmd.enabled([] { return false; });
    CHECK(is_visible_and_enabled(cmd, Visibility::Both) == false);
    CHECK(is_visible_and_enabled(cmd, Visibility::Repl) == false);
    CHECK(is_visible_and_enabled(cmd, Visibility::Cli) == false);
}

TEST_CASE("collect_options_in_chain returns current then ancestors")
{
    App app("test", "1.0", "Chain");
    app.option<fixed_string("verbose")>("--verbose", 'v', "Verbose").boolean();
    auto &leaf = app.add_leaf("serve", "Serve");
    leaf.option<fixed_string("port")>("--port", 'p', "Port").integer();

    auto chain = collect_options_in_chain(leaf);
    REQUIRE(chain.size() == 2);
    CHECK(chain[0].opt->long_name() == "port");
    CHECK(chain[1].opt->long_name() == "verbose");
}

TEST_CASE("collect_options_in_chain marks shadowed long name")
{
    App app("test", "1.0", "Chain");
    app.option<fixed_string("opt")>("--opt", 'o', "Root").boolean();
    auto &child = app.add_leaf("child", "Child");
    child.option<fixed_string("opt")>("--opt", "Leaf").boolean();

    auto chain = collect_options_in_chain(child);
    REQUIRE(chain.size() == 2);
    CHECK_FALSE(chain[0].long_shadowed);
    CHECK(chain[1].long_shadowed);
    CHECK_FALSE(chain[1].short_shadowed);
    CHECK(chain[1].opt->short_name() == 'o');
}

TEST_CASE("collect_options_in_chain skips fully shadowed option")
{
    App app("test", "1.0", "Chain");
    app.option<fixed_string("opt")>("--opt", "Root").boolean();
    auto &child = app.add_leaf("child", "Child");
    child.option<fixed_string("opt")>("--opt", "Leaf").boolean();

    CHECK(collect_options_in_chain(child).size() == 1);
}

TEST_CASE("collect_options_in_chain excludes current when asked")
{
    App app("test", "1.0", "Chain");
    app.option<fixed_string("opt")>("--opt", 'o', "Root").boolean();
    auto &child = app.add_leaf("child", "Child");
    child.option<fixed_string("opt")>("--opt", "Leaf").boolean();

    auto chain = collect_options_in_chain(child, false);
    REQUIRE(chain.size() == 1);
    CHECK(chain[0].opt->short_name() == 'o');
    CHECK(chain[0].long_shadowed);
}
