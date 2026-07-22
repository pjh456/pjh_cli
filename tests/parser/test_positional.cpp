#include <doctest/doctest.h>

#include <iostream>
#include <pjh_cli/command/base_command.hpp>
#include <pjh_cli/command/leaf_command.hpp>
#include <pjh_cli/core/fixed_string.hpp>
#include <pjh_cli/parse/parser.hpp>

#include "test_helpers.hpp"

TEST_CASE("Parser positional args")
{
    LeafCommand root("test", "Positional test");
    root.arg<std::string, 0>("source", "Source file");
    root.arg<std::string, 1>("dest", "Destination");
    Argv argv{"test", "in.txt", "out.txt"};
    auto r = Parser::parse_command(root, argv.argc(), argv.argv());
    CHECK(r.is_ok());
    auto &ctx = r.unwrap();
    CHECK(ctx.get<std::string, 0>() == "in.txt");
    CHECK(ctx.get<std::string, 1>() == "out.txt");
}

TEST_CASE("Parser mixed options and positional args")
{
    LeafCommand root("test", "Mixed test");
    root.option<fixed_string("verbose")>("--verbose", 'v', "Verbose").boolean();
    root.arg<std::string, 0>("file", "Input file");
    Argv argv{"test", "--verbose", "data.txt"};
    auto r = Parser::parse_command(root, argv.argc(), argv.argv());
    CHECK(r.is_ok());
    auto &ctx = r.unwrap();
    CHECK(ctx.get<bool, fixed_string("verbose")>() == true);
    CHECK(ctx.get<std::string, 0>() == "data.txt");
}

TEST_CASE("Parser required arg insufficient args")
{
    LeafCommand root("test", "Insufficient required");
    root.arg<std::string, 0>("src", "Source").required();
    root.arg<std::string, 1>("dst", "Dest").required();
    Argv argv{"test", "onlysrc"};
    auto r = Parser::parse_command(root, argv.argc(), argv.argv());
    CHECK(r.is_err());
}

TEST_CASE("Parser optional arg insufficient does not fail")
{
    LeafCommand root("test", "Insufficient optional");
    root.arg<std::string, 0>("src", "Source");
    root.arg<std::string, 1>("dst", "Dest");
    Argv argv{"test", "onlysrc"};
    auto r = Parser::parse_command(root, argv.argc(), argv.argv());
    CHECK(r.is_ok());
}

TEST_CASE("Parser all positional store with no registered args")
{
    LeafCommand root("test", "All positional store");
    root.set_extra_args(ExtraArgsPolicy::Store);
    Argv argv{"test", "a.txt", "b.txt", "c.txt"};
    auto r = Parser::parse_command(root, argv.argc(), argv.argv());
    CHECK(r.is_ok());
    auto extra = r.unwrap().extra_args();
    CHECK(extra.size() == 3);
    CHECK(extra[0] == "a.txt");
    CHECK(extra[1] == "b.txt");
    CHECK(extra[2] == "c.txt");
}

TEST_CASE("Parser all positional ignore with no registered args")
{
    LeafCommand root("test", "All positional ignore");
    Argv argv{"test", "a.txt", "b.txt"};
    auto r = Parser::parse_command(root, argv.argc(), argv.argv());
    CHECK(r.is_ok());
}

TEST_CASE("Parser all positional error with no registered args")
{
    LeafCommand root("test", "All positional error");
    root.set_extra_args(ExtraArgsPolicy::Error);
    Argv argv{"test", "a.txt"};
    auto r = Parser::parse_command(root, argv.argc(), argv.argv());
    CHECK(r.is_err());
}

TEST_CASE("Parser extra positional args ignore")
{
    LeafCommand root("test", "Extra args test");
    root.arg<std::string, 0>("file", "Input file");
    Argv argv{"test", "a.txt", "b.txt", "c.txt"};
    auto r = Parser::parse_command(root, argv.argc(), argv.argv());
    CHECK(r.is_ok());
    CHECK(r.unwrap().get<std::string, 0>() == "a.txt");
}

TEST_CASE("Parser extra positional args error policy")
{
    LeafCommand root("test", "Strict error test");
    root.set_extra_args(ExtraArgsPolicy::Error);
    root.arg<std::string, 0>("file", "Input file");
    Argv argv{"test", "a.txt", "b.txt"};
    auto r = Parser::parse_command(root, argv.argc(), argv.argv());
    CHECK(r.is_err());
}

TEST_CASE("Parser extra positional args store policy")
{
    LeafCommand root("test", "Store test");
    root.set_extra_args(ExtraArgsPolicy::Store);
    root.arg<std::string, 0>("file", "Input file");
    Argv argv{"test", "a.txt", "b.txt", "c.txt"};
    auto r = Parser::parse_command(root, argv.argc(), argv.argv());
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
    LeafCommand root("test", "Store no extra");
    root.set_extra_args(ExtraArgsPolicy::Store);
    root.arg<std::string, 0>("file", "Input file");
    Argv argv{"test", "a.txt"};
    auto r = Parser::parse_command(root, argv.argc(), argv.argv());
    CHECK(r.is_ok());
    CHECK(r.unwrap().extra_args().empty());
}
