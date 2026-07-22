#include <doctest/doctest.h>

#include <pjh_cli/app.hpp>
#include <pjh_cli/command/base_command.hpp>
#include <pjh_cli/command/leaf_command.hpp>
#include <pjh_cli/fixed_string.hpp>
#include <pjh_cli/parser.hpp>
#include "test_helpers.hpp"

TEST_CASE("Parser double dash separator")
{
    LeafCommand root("test", "Double dash test");
    root.option<fixed_string("verbose")>("--verbose", 'v', "Verbose").boolean();
    root.arg<std::string, 0>("file", "Input file");
    Argv argv{"test", "--verbose", "--", "--file=foo"};
    auto r = Parser::parse_command(root, argv.argc(), argv.argv());
    CHECK(r.is_ok());
    auto &ctx = r.unwrap();
    CHECK(ctx.get<bool, fixed_string("verbose")>() == true);
    CHECK(ctx.get<std::string, 0>() == "--file=foo");
}

TEST_CASE("Parser double dash with no further args")
{
    App app("test", "1.0", "DD empty");
    app.option<fixed_string("verbose")>("--verbose", 'v', "Verbose").boolean();
    Argv argv{"test", "--verbose", "--"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_ok());
    CHECK(r.unwrap().get<bool, fixed_string("verbose")>() == true);
}

TEST_CASE("Parser double dash with ExtraArgsPolicy::Error")
{
    LeafCommand root("test", "DD error");
    root.set_extra_args(ExtraArgsPolicy::Error);
    root.arg<std::string, 0>("file", "Input file");
    Argv argv{"test", "--", "a.txt", "b.txt"};
    auto r = Parser::parse_command(root, argv.argc(), argv.argv());
    CHECK(r.is_err());
}
