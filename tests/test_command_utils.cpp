#include <pjh_cli.hpp>
#include <pjh_cli/detail/command_utils.hpp>
#include <doctest/doctest.h>
#include <string>
#include <string_view>

using namespace pjh::cli;
using namespace pjh::cli::detail;

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

TEST_CASE("option_left_label long only no value")
{
    OptionDef opt;
    opt.m_long_name = "verbose";
    opt.m_has_value = false;
    CHECK(option_left_label(opt) == "--verbose");
}

TEST_CASE("option_left_label long only with value")
{
    OptionDef opt;
    opt.m_long_name = "port";
    opt.m_has_value = true;
    CHECK(option_left_label(opt) == "--port PORT");
}

TEST_CASE("option_left_label short only no value")
{
    OptionDef opt;
    opt.m_short_name = 'v';
    opt.m_has_value = false;
    CHECK(option_left_label(opt) == "-v");
}

TEST_CASE("option_left_label short only with value")
{
    OptionDef opt;
    opt.m_short_name = 'p';
    opt.m_has_value = true;
    CHECK(option_left_label(opt) == "-p P");
}

TEST_CASE("option_left_label both no value")
{
    OptionDef opt;
    opt.m_long_name = "verbose";
    opt.m_short_name = 'v';
    opt.m_has_value = false;
    CHECK(option_left_label(opt) == "-v, --verbose");
}

TEST_CASE("option_left_label both with value")
{
    OptionDef opt;
    opt.m_long_name = "port";
    opt.m_short_name = 'p';
    opt.m_has_value = true;
    CHECK(option_left_label(opt) == "-p, --port PORT");
}

TEST_CASE("option_left_label both with custom separator")
{
    OptionDef opt;
    opt.m_long_name = "verbose";
    opt.m_short_name = 'v';
    opt.m_has_value = false;
    CHECK(option_left_label(opt, "|") == "-v|--verbose");
}

TEST_CASE("option_left_label empty")
{
    OptionDef opt;
    CHECK(option_left_label(opt).empty());
}
