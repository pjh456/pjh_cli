#include <doctest/doctest.h>

#include <iostream>
#include <pjh_cli/app.hpp>
#include <pjh_cli/core/fixed_string.hpp>
#include <pjh_cli/parse/parser.hpp>
#include <string_view>

#include "test_helpers.hpp"

TEST_CASE("ExactlyOne group with none provided errors")
{
    App app("test", "1.0", "Group test");
    app.option<fixed_string("port")>("--port", "Port").integer();
    app.option<fixed_string("socket")>("--socket", "Socket").str();
    app.group<fixed_string("port"), fixed_string("socket")>().exactly_one();

    Argv argv{"test"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_err());
    CHECK(
        r.unwrap_err().what() ==
        std::string_view("Parse Error: exactly one of --port, --socket is required"));
}

TEST_CASE("ExactlyOne group with one provided succeeds")
{
    App app("test", "1.0", "Group test");
    app.option<fixed_string("port")>("--port", "Port").integer();
    app.option<fixed_string("socket")>("--socket", "Socket").str();
    app.group<fixed_string("port"), fixed_string("socket")>().exactly_one();

    Argv argv{"test", "--port", "8080"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_ok());
    CHECK(r.unwrap().get<int, fixed_string("port")>() == 8080);
}

TEST_CASE("ExactlyOne group with both provided errors")
{
    App app("test", "1.0", "Group test");
    app.option<fixed_string("port")>("--port", "Port").integer();
    app.option<fixed_string("socket")>("--socket", "Socket").str();
    app.group<fixed_string("port"), fixed_string("socket")>().exactly_one();

    Argv argv{"test", "--port", "8080", "--socket", "/tmp/s"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_err());
    CHECK(
        r.unwrap_err().what() == std::string_view(
                                     "Parse Error: conflicting options: --port, --socket "
                                     "cannot be used together"));
}

TEST_CASE("AtMostOne group with none provided succeeds")
{
    App app("test", "1.0", "Group test");
    app.option<fixed_string("verbose")>("--verbose", 'v', "Verbose").boolean();
    app.option<fixed_string("quiet")>("--quiet", 'q', "Quiet").boolean();
    app.group<fixed_string("verbose"), fixed_string("quiet")>().at_most_one();

    Argv argv{"test"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_ok());
}

TEST_CASE("AtMostOne group with one provided succeeds")
{
    App app("test", "1.0", "Group test");
    app.option<fixed_string("verbose")>("--verbose", 'v', "Verbose").boolean();
    app.option<fixed_string("quiet")>("--quiet", 'q', "Quiet").boolean();
    app.group<fixed_string("verbose"), fixed_string("quiet")>().at_most_one();

    Argv argv{"test", "--verbose"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_ok());
    CHECK(r.unwrap().get<bool, fixed_string("verbose")>() == true);
}

TEST_CASE("AtMostOne group with both provided errors")
{
    App app("test", "1.0", "Group test");
    app.option<fixed_string("verbose")>("--verbose", 'v', "Verbose").boolean();
    app.option<fixed_string("quiet")>("--quiet", 'q', "Quiet").boolean();
    app.group<fixed_string("verbose"), fixed_string("quiet")>().at_most_one();

    Argv argv{"test", "--verbose", "--quiet"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_err());
    CHECK(
        r.unwrap_err().what() == std::string_view(
                                     "Parse Error: conflicting options: --verbose, "
                                     "--quiet cannot be used together"));
}

TEST_CASE("AtLeastOne group with none provided errors")
{
    App app("test", "1.0", "Group test");
    app.option<fixed_string("token_file")>("--token-file", "Token file").str();
    app.option<fixed_string("token_env")>("--token-env", "Token env").str();
    app.group<fixed_string("token_file"), fixed_string("token_env")>().at_least_one();

    Argv argv{"test"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_err());
    CHECK(
        r.unwrap_err().what() ==
        std::string_view(
            "Parse Error: at least one of --token-file, --token-env is required"));
}

TEST_CASE("AtLeastOne group with one provided succeeds")
{
    App app("test", "1.0", "Group test");
    app.option<fixed_string("token_file")>("--token-file", "Token file").str();
    app.option<fixed_string("token_env")>("--token-env", "Token env").str();
    app.group<fixed_string("token_file"), fixed_string("token_env")>().at_least_one();

    Argv argv{"test", "--token-file", "/tmp/t"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_ok());
}

TEST_CASE("Group with default value satisfies ExactlyOne")
{
    App app("test", "1.0", "Group default test");
    app.option<fixed_string("port")>("--port", "Port", 3000);
    app.option<fixed_string("socket")>("--socket", "Socket").str();
    app.group<fixed_string("port"), fixed_string("socket")>().exactly_one();

    Argv argv{"test"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_ok());
    CHECK(r.unwrap().get<int, fixed_string("port")>() == 3000);
}

TEST_CASE("Group in subcommand validates correctly")
{
    App app("test", "1.0", "Group subcommand test");
    auto &cmd = app.add_leaf("deploy", "Deploy");
    cmd.option<fixed_string("region")>("--region", "Region").str();
    cmd.option<fixed_string("zone")>("--zone", "Zone").str();
    cmd.group<fixed_string("region"), fixed_string("zone")>().exactly_one();

    Argv argv{"test", "deploy"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_err());
}

TEST_CASE("Group with three options exactly_one")
{
    App app("test", "1.0", "Group three test");
    app.option<fixed_string("a")>("--a", "A").str();
    app.option<fixed_string("b")>("--b", "B").str();
    app.option<fixed_string("c")>("--c", "C").str();
    app.group<fixed_string("a"), fixed_string("b"), fixed_string("c")>().exactly_one();

    Argv argv{"test", "--a", "x", "--b", "y"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_err());
}
