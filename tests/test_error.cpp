#include <doctest/doctest.h>

#include <iostream>
#include <pjh_cli/core/error.hpp>
#include <stdexcept>
#include <string_view>
#include <variant>
#include <vector>

using namespace pjh::cli;

TEST_CASE("CliError basic")
{
    CliError e("something went wrong");
    CHECK(std::string_view(e.what()) == "something went wrong");
    CHECK(e.kind() == ErrorKind::Runtime);
}

TEST_CASE("CliError empty message")
{
    CliError e("");
    CHECK(std::string_view(e.what()) == "");
    CHECK(e.kind() == ErrorKind::Runtime);
}

TEST_CASE("CliError from char pointer")
{
    CliError e("bad input");
    CHECK(std::string_view(e.what()) == "bad input");
    CHECK(e.kind() == ErrorKind::Runtime);
}

TEST_CASE("CliError is runtime_error")
{
    CliError e("test");
    const std::runtime_error &base = e;
    CHECK(std::string_view(base.what()) == "test");
    CHECK(e.kind() == ErrorKind::Runtime);
}

TEST_CASE("CliError plain message is runtime kind")
{
    CliError e("boom");
    CHECK(std::string_view(e.what()) == "boom");
    CHECK(e.kind() == ErrorKind::Runtime);
}

TEST_CASE("ErrorFactory runtime_error omits parse prefix")
{
    auto e = ErrorFactory::runtime_error("读取存档失败");
    CHECK(std::string_view(e.what()) == "读取存档失败");
    CHECK(e.kind() == ErrorKind::Runtime);
}

TEST_CASE("ErrorFactory parse errors are parse kind")
{
    auto unknown = ErrorFactory::unknown_option("--bogus");
    CHECK(std::string_view(unknown.what()) == "Parse Error: unknown option: '--bogus'");
    CHECK(unknown.kind() == ErrorKind::Parse);

    auto missing = ErrorFactory::missing_value("--port");
    CHECK(
        std::string_view(missing.what()) ==
        "Parse Error: option '--port' requires a value");
    CHECK(missing.kind() == ErrorKind::Parse);
}

TEST_CASE("CliError ErrorInfo defaults to parse kind")
{
    CliError e(ErrorInfo(RawMessageError{"raw"}));
    CHECK(std::string_view(e.what()) == "Parse Error: raw");
    CHECK(e.kind() == ErrorKind::Parse);
}

TEST_CASE("CliError explicit runtime kind omits prefix")
{
    CliError e(UnknownOptionError{"--x"}, ErrorKind::Runtime);
    CHECK(std::string_view(e.what()) == "unknown option: '--x'");
    CHECK(e.kind() == ErrorKind::Runtime);
}

TEST_CASE("CliError copy preserves kind")
{
    CliError original("boom");
    CliError copy = original;
    CHECK(copy.kind() == ErrorKind::Runtime);
    CHECK(std::string_view(copy.what()) == "boom");
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
    auto e = ErrorFactory::parse_error("--port", 5);
    CHECK(
        std::string_view(e.what()) ==
        "Parse Error: unexpected argument '--port' at position 5");
}

TEST_CASE("parse_error negative position")
{
    auto e = ErrorFactory::parse_error("arg", -1);
    CHECK(
        std::string_view(e.what()) ==
        "Parse Error: unexpected argument 'arg' at position -1");
}

TEST_CASE("parse error renders the prefix exactly once")
{
    auto e = ErrorFactory::parse_error("x", 1);
    const std::string_view what = e.what();
    CHECK(what == "Parse Error: unexpected argument 'x' at position 1");
    CHECK(what.starts_with("Parse Error: "));
    CHECK(what.find("parse error") == std::string_view::npos);
    CHECK(e.kind() == ErrorKind::Parse);
    CHECK(e.tag() == ErrorTag::Parse);
    CHECK(format_error(e.info()) == "unexpected argument 'x' at position 1");
}

TEST_CASE("unknown_option format")
{
    auto e = ErrorFactory::unknown_option("--bogus");
    CHECK(std::string_view(e.what()) == "Parse Error: unknown option: '--bogus'");
}

TEST_CASE("unknown_option empty name")
{
    auto e = ErrorFactory::unknown_option("");
    CHECK(std::string_view(e.what()) == "Parse Error: unknown option: ''");
}

TEST_CASE("unknown_option with suggestions")
{
    auto e = ErrorFactory::unknown_option("--prot", {"--port"});
    CHECK(
        std::string_view(e.what()) ==
        "Parse Error: unknown option: '--prot'; did you mean: --port");
    CHECK(e.kind() == ErrorKind::Parse);
}

TEST_CASE("unknown_option with multiple suggestions")
{
    auto e = ErrorFactory::unknown_option("--prot", {"--ports", "--port"});
    CHECK(
        std::string_view(e.what()) ==
        "Parse Error: unknown option: '--prot'; did you mean: --ports, --port");
}

TEST_CASE("missing_value format")
{
    auto e = ErrorFactory::missing_value("--port");
    CHECK(std::string_view(e.what()) == "Parse Error: option '--port' requires a value");
}

TEST_CASE("missing_value single char")
{
    auto e = ErrorFactory::missing_value("-x");
    CHECK(std::string_view(e.what()) == "Parse Error: option '-x' requires a value");
}

TEST_CASE("missing_required_option format")
{
    auto e = ErrorFactory::missing_required_option("port");
    CHECK(std::string_view(e.what()) == "Parse Error: missing required option: 'port'");
}

TEST_CASE("missing_required_arg format")
{
    auto e = ErrorFactory::missing_required_arg("file");
    CHECK(std::string_view(e.what()) == "Parse Error: missing required argument: 'file'");
}

TEST_CASE("type_conversion_error format")
{
    auto e = ErrorFactory::type_conversion_error("--port", "abc", "integer");
    CHECK(
        std::string_view(e.what()) ==
        "Parse Error: invalid value 'abc' for '--port': expected integer");
}

TEST_CASE("type_conversion_error empty parts")
{
    auto e = ErrorFactory::type_conversion_error("", "", "");
    CHECK(
        std::string_view(e.what()) == "Parse Error: invalid value '' for '': expected ");
}

TEST_CASE("enum_value_error format")
{
    auto e = ErrorFactory::enum_value_error("--color", "yellow", {"red", "green"});
    CHECK(
        std::string_view(e.what()) ==
        "Parse Error: invalid value 'yellow' for '--color': expected one of: red, green");
}

TEST_CASE("ambiguous_command format")
{
    auto e = ErrorFactory::ambiguous_command("st", {"start", "stop"});
    auto msg = std::string_view(e.what());
    CHECK(
        msg.find("Parse Error: ambiguous command 'st', candidates:") !=
        std::string_view::npos);
    CHECK(msg.find("start") != std::string_view::npos);
    CHECK(msg.find("stop") != std::string_view::npos);
}

TEST_CASE("ambiguous_command empty candidates")
{
    auto e = ErrorFactory::ambiguous_command("x", {});
    CHECK(
        std::string_view(e.what()) == "Parse Error: ambiguous command 'x', candidates:");
}

TEST_CASE("ambiguous_command single candidate")
{
    auto e = ErrorFactory::ambiguous_command("ser", {"server"});
    auto msg = std::string_view(e.what());
    CHECK(msg.find("server") != std::string_view::npos);
}

TEST_CASE("unknown_command format")
{
    auto e = ErrorFactory::unknown_command("instal", {"install"});
    CHECK(
        std::string_view(e.what()) ==
        "Parse Error: unknown command: 'instal'; did you mean: install");
}

TEST_CASE("unknown_command with suggestions")
{
    auto e = ErrorFactory::unknown_command("st", {"start", "stop"});
    auto msg = std::string_view(e.what());
    CHECK(msg.find("Parse Error: unknown command: 'st'") != std::string_view::npos);
    CHECK(msg.find("did you mean: start, stop") != std::string_view::npos);
}

TEST_CASE("unknown_command empty suggestions")
{
    auto e = ErrorFactory::unknown_command("zzzz", {});
    CHECK(std::string_view(e.what()) == "Parse Error: unknown command: 'zzzz'");
}

TEST_CASE("command_disabled format")
{
    auto e = ErrorFactory::command_disabled("oldcmd");
    CHECK(std::string_view(e.what()) == "Parse Error: command 'oldcmd' is not available");
}

TEST_CASE("command_disabled empty")
{
    auto e = ErrorFactory::command_disabled("");
    CHECK(std::string_view(e.what()) == "Parse Error: command '' is not available");
}

TEST_CASE("format_error covers every ErrorInfo alternative")
{
    const std::vector<ErrorInfo> all = {
        RawMessageError{"raw"},
        ParseError{"--x", 1},
        UnknownOptionError{"--x"},
        MissingValueError{"--x"},
        MissingRequiredOptionError{"--x"},
        MissingRequiredArgError{"x"},
        TypeConversionError{"--x", "v", "integer"},
        AmbiguousCommandError{"x", {"a", "b"}},
        UnknownCommandError{"x", {"y"}},
        ValueOutOfRangeError{"--x", "9", "0", "5"},
        EnumValueError{"--x", "v", {"a"}},
        CommandDisabledError{"x"},
        ConflictingOptionsError{{"a", "b"}},
        RequiredOptionGroupError{{"a"}, true},
        OptionDoesNotAcceptValueError{"--x"},
        NoCommandMatchedError{},
    };
    REQUIRE(all.size() == 16u);
    for (const auto &info : all)
    {
        CliError e(info);
        CHECK(std::string_view(e.what()).starts_with("Parse Error: "));
    }
}

TEST_CASE("format_error returns message without prefix")
{
    CHECK(format_error(RawMessageError{"raw"}) == "raw");
    CHECK(format_error(UnknownOptionError{"--x"}) == "unknown option: '--x'");
}

TEST_CASE("value_out_of_range int format")
{
    auto e = ErrorFactory::value_out_of_range("--port", "9", 0, 5);
    CHECK(
        std::string_view(e.what()) ==
        "Parse Error: value '9' for '--port' is out of range [0, 5]");
    CHECK(e.kind() == ErrorKind::Parse);
}

TEST_CASE("value_out_of_range double format")
{
    auto e = ErrorFactory::value_out_of_range("--ratio", "9.5", 0.0, 1.0);
    CHECK(
        std::string_view(e.what()) ==
        "Parse Error: value '9.5' for '--ratio' is out of range [0, 1]");
}

TEST_CASE("conflicting_options format")
{
    auto e = ErrorFactory::conflicting_options({"a", "b"});
    CHECK(
        std::string_view(e.what()) ==
        "Parse Error: conflicting options: a, b cannot be used together");
}

TEST_CASE("required_option_group exactly one format")
{
    auto e = ErrorFactory::required_option_group({"a", "b"}, true);
    CHECK(std::string_view(e.what()) == "Parse Error: exactly one of a, b is required");
}

TEST_CASE("required_option_group at least one format")
{
    auto e = ErrorFactory::required_option_group({"a", "b"}, false);
    CHECK(std::string_view(e.what()) == "Parse Error: at least one of a, b is required");
}

TEST_CASE("option_does_not_accept_value format")
{
    auto e = ErrorFactory::option_does_not_accept_value("--flag");
    CHECK(
        std::string_view(e.what()) ==
        "Parse Error: option '--flag' does not accept a value");
}

TEST_CASE("no_command_matched format")
{
    auto e = ErrorFactory::no_command_matched();
    CHECK(std::string_view(e.what()) == "Parse Error: no command matched");
    CHECK(e.kind() == ErrorKind::Parse);
}

TEST_CASE("expected_type_name maps builtin tags to canonical text")
{
    CHECK(expected_type_name(ExpectedType::Integer) == "integer");
    CHECK(expected_type_name(ExpectedType::Float) == "float");
    CHECK(expected_type_name(ExpectedType::Bool) == "bool (true/false/yes/no/1/0)");
    CHECK(expected_type_name(ExpectedType::Unknown).empty());
    CHECK(expected_type_name(ExpectedType::Count).empty());
}

TEST_CASE("expected_type_from_string best-effort maps canonical text")
{
    CHECK(expected_type_from_string("integer") == ExpectedType::Integer);
    CHECK(expected_type_from_string("float") == ExpectedType::Float);
    CHECK(
        expected_type_from_string("bool (true/false/yes/no/1/0)") == ExpectedType::Bool);
    CHECK(expected_type_from_string("whatever") == ExpectedType::Unknown);
    CHECK(expected_type_from_string("") == ExpectedType::Unknown);
}

TEST_CASE("type_conversion_error tag overload derives display text")
{
    auto e = ErrorFactory::type_conversion_error("--port", "abc", ExpectedType::Integer);
    const auto *info = std::get_if<TypeConversionError>(&e.info());
    REQUIRE(info != nullptr);
    CHECK(info->expected == ExpectedType::Integer);
    CHECK(info->expected_type == "integer");
    CHECK(
        std::string_view(e.what()) ==
        "Parse Error: invalid value 'abc' for '--port': expected integer");
}

TEST_CASE("type_conversion_error string overload classifies canonical text")
{
    auto e = ErrorFactory::type_conversion_error("--x", "v", "integer");
    const auto *info = std::get_if<TypeConversionError>(&e.info());
    REQUIRE(info != nullptr);
    CHECK(info->expected == ExpectedType::Integer);
    CHECK(info->expected_type == "integer");
}

TEST_CASE("type_conversion_error custom display text stays Unknown")
{
    auto e = ErrorFactory::type_conversion_error("--x", "v", "whatever");
    const auto *info = std::get_if<TypeConversionError>(&e.info());
    REQUIRE(info != nullptr);
    CHECK(info->expected == ExpectedType::Unknown);
    CHECK(info->expected_type == "whatever");
    CHECK(
        std::string_view(e.what()) ==
        "Parse Error: invalid value 'v' for '--x': expected whatever");
}

TEST_CASE("type_conversion_error tag and display text stay consistent")
{
    for (auto tag : {ExpectedType::Integer, ExpectedType::Float, ExpectedType::Bool})
    {
        auto e = ErrorFactory::type_conversion_error("--x", "v", tag);
        const auto *info = std::get_if<TypeConversionError>(&e.info());
        REQUIRE(info != nullptr);
        CHECK(info->expected_type == expected_type_name(info->expected));
    }
}
