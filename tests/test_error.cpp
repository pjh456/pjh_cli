#include <doctest/doctest.h>

#include <pjh_cli.hpp>
#include <string>
#include <string_view>

using namespace pjh::cli;

TEST_CASE("CliError basic")
{
    CliError e("something went wrong");
    CHECK(std::string_view(e.what()) == "Parse Error: something went wrong");
}

TEST_CASE("CliError empty message")
{
    CliError e("");
    CHECK(std::string_view(e.what()) == "Parse Error: ");
}

TEST_CASE("CliError from char pointer")
{
    CliError e("bad input");
    CHECK(std::string_view(e.what()) == "Parse Error: bad input");
}

TEST_CASE("CliError is runtime_error")
{
    CliError e("test");
    const std::runtime_error &base = e;
    CHECK(std::string_view(base.what()) == "Parse Error: test");
}

TEST_CASE("LogicError basic")
{
    LogicError e("programming mistake");
    CHECK(std::string_view(e.what()) == "programming mistake");
}

TEST_CASE("LogicError is logic_error")
{
    LogicError e("logic fail");
    const std::logic_error &base = e;
    CHECK(std::string_view(base.what()) == "logic fail");
}

TEST_CASE("parse_error format")
{
    auto e = parse_error("--port", 5);
    CHECK(
        std::string_view(e.what()) ==
        "Parse Error: parse error at argument '--port', position 5");
}

TEST_CASE("parse_error negative position")
{
    auto e = parse_error("arg", -1);
    CHECK(
        std::string_view(e.what()) ==
        "Parse Error: parse error at argument 'arg', position -1");
}

TEST_CASE("unknown_option format")
{
    auto e = unknown_option("--bogus");
    CHECK(std::string_view(e.what()) == "Parse Error: unknown option: '--bogus'");
}

TEST_CASE("unknown_option empty name")
{
    auto e = unknown_option("");
    CHECK(std::string_view(e.what()) == "Parse Error: unknown option: ''");
}

TEST_CASE("missing_value format")
{
    auto e = missing_value("--port");
    CHECK(std::string_view(e.what()) == "Parse Error: option '--port' requires a value");
}

TEST_CASE("missing_value single char")
{
    auto e = missing_value("-x");
    CHECK(std::string_view(e.what()) == "Parse Error: option '-x' requires a value");
}

TEST_CASE("missing_required_option format")
{
    auto e = missing_required_option("port");
    CHECK(std::string_view(e.what()) == "Parse Error: missing required option: 'port'");
}

TEST_CASE("missing_required_arg format")
{
    auto e = missing_required_arg("file");
    CHECK(std::string_view(e.what()) == "Parse Error: missing required argument: 'file'");
}

TEST_CASE("type_conversion_error format")
{
    auto e = type_conversion_error("--port", "abc", "integer");
    CHECK(
        std::string_view(e.what()) ==
        "Parse Error: invalid value 'abc' for '--port': expected integer");
}

TEST_CASE("type_conversion_error empty parts")
{
    auto e = type_conversion_error("", "", "");
    CHECK(
        std::string_view(e.what()) == "Parse Error: invalid value '' for '': expected ");
}

TEST_CASE("ambiguous_command format")
{
    auto e = ambiguous_command("st", {"start", "stop"});
    auto msg = std::string_view(e.what());
    CHECK(
        msg.find("Parse Error: ambiguous command 'st', candidates:") !=
        std::string_view::npos);
    CHECK(msg.find("start") != std::string_view::npos);
    CHECK(msg.find("stop") != std::string_view::npos);
}

TEST_CASE("ambiguous_command empty candidates")
{
    auto e = ambiguous_command("x", {});
    CHECK(
        std::string_view(e.what()) == "Parse Error: ambiguous command 'x', candidates:");
}

TEST_CASE("ambiguous_command single candidate")
{
    auto e = ambiguous_command("ser", {"server"});
    auto msg = std::string_view(e.what());
    CHECK(msg.find("server") != std::string_view::npos);
}

TEST_CASE("command_disabled format")
{
    auto e = command_disabled("oldcmd");
    CHECK(std::string_view(e.what()) == "Parse Error: command 'oldcmd' is not available");
}

TEST_CASE("command_disabled empty")
{
    auto e = command_disabled("");
    CHECK(std::string_view(e.what()) == "Parse Error: command '' is not available");
}
