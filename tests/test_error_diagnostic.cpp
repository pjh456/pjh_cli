#include <doctest/doctest.h>

#include <pjh_cli/core/error.hpp>
#include <pjh_result.hpp>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

using namespace pjh::cli;

namespace
{
    /// @brief Stand-in for a downstream localisation formatter: switches on the
    ///        stable ErrorTag and falls back to the English message for any tag
    ///        it does not map, so a new tag never silently disappears.
    std::string localize(const ErrorDiagnostic &d)
    {
        switch (d.kind())
        {
        case ErrorTag::Parse:
            return "解析错误";
        case ErrorTag::UnknownOption:
            return "未知选项";
        case ErrorTag::MissingValue:
            return "缺少选项值";
        default:
            return std::string(d.message());
        }
    }
}  // namespace

TEST_CASE("ErrorFactory producers map to their ErrorTag")
{
    const std::vector<std::pair<ErrorTag, CliError>> cases = {
        {ErrorTag::Parse, ErrorFactory::parse_error("--port", 5)},
        {ErrorTag::UnknownOption, ErrorFactory::unknown_option("--bogus")},
        {ErrorTag::MissingValue, ErrorFactory::missing_value("--port")},
        {ErrorTag::MissingRequiredOption, ErrorFactory::missing_required_option("port")},
        {ErrorTag::MissingRequiredArg, ErrorFactory::missing_required_arg("file")},
        {ErrorTag::TypeConversion,
         ErrorFactory::type_conversion_error("--port", "abc", "integer")},
        {ErrorTag::AmbiguousCommand,
         ErrorFactory::ambiguous_command("st", {"start", "stop"})},
        {ErrorTag::UnknownCommand, ErrorFactory::unknown_command("instal", {"install"})},
        {ErrorTag::ValueOutOfRange,
         ErrorFactory::value_out_of_range("--port", "9", 0, 5)},
        {ErrorTag::EnumValue,
         ErrorFactory::enum_value_error("--color", "yellow", {"red"})},
        {ErrorTag::CommandDisabled, ErrorFactory::command_disabled("old")},
        {ErrorTag::ConflictingOptions, ErrorFactory::conflicting_options({"a", "b"})},
        {ErrorTag::RequiredOptionGroup,
         ErrorFactory::required_option_group({"a", "b"}, true)},
        {ErrorTag::OptionDoesNotAcceptValue,
         ErrorFactory::option_does_not_accept_value("--flag")},
        {ErrorTag::NoCommandMatched, ErrorFactory::no_command_matched()},
    };
    REQUIRE(cases.size() == 15u);
    for (const auto &[expected, err] : cases)
    {
        CHECK(err.tag() == expected);
        CHECK(err.diagnostic().kind() == expected);
        CHECK(err.kind() == ErrorKind::Parse);
    }
}

TEST_CASE("parse RawMessageError maps to RawMessage and not Runtime")
{
    CliError e(ErrorInfo(RawMessageError{"raw"}));
    CHECK(e.tag() == ErrorTag::RawMessage);
    CHECK(e.kind() == ErrorKind::Parse);
}

TEST_CASE("runtime errors map to Runtime regardless of payload")
{
    CliError plain("boom");
    CHECK(plain.tag() == ErrorTag::Runtime);

    CliError from_string(std::string("boom"));
    CHECK(from_string.tag() == ErrorTag::Runtime);

    auto factory = ErrorFactory::runtime_error("boom");
    CHECK(factory.tag() == ErrorTag::Runtime);

    CliError explicit_runtime(UnknownOptionError{"--x"}, ErrorKind::Runtime);
    CHECK(explicit_runtime.tag() == ErrorTag::Runtime);
}

TEST_CASE("CliError and ErrorDiagnostic satisfy pjh_result Diagnostic")
{
    static_assert(pjh::result::Diagnostic<CliError>);
    static_assert(pjh::result::Diagnostic<ErrorDiagnostic>);

    auto e = ErrorFactory::unknown_option("--bogus");
    CHECK(e.message() == std::string_view(e.what()));
    CHECK(pjh::result::render(e) == e.what());
    CHECK(pjh::result::render(e.diagnostic()) == e.what());
    CHECK(e.diagnostic().error_kind() == e.kind());
    CHECK(e.diagnostic().info().index() == e.info().index());
}

TEST_CASE("Context<CliError> renders the what() text under a context prefix")
{
    auto e = ErrorFactory::missing_value("--port");
    auto ctx = pjh::result::Context<CliError>(e).context("load");

    const std::string rendered = pjh::result::render(ctx);
    CHECK(rendered == std::string("load: ") + e.what());
    CHECK(rendered.starts_with("load: "));
    CHECK(ctx.message() == std::string_view("load"));
    CHECK(ctx.kind() == e.kind());
}

TEST_CASE("localizer maps ErrorTag and falls back to the message")
{
    auto known = ErrorFactory::unknown_option("--bogus");
    CHECK(localize(known.diagnostic()) == "未知选项");

    auto parsed = ErrorFactory::parse_error("--x", 1);
    CHECK(localize(parsed.diagnostic()) == "解析错误");

    auto missing = ErrorFactory::missing_value("--port");
    CHECK(localize(missing.diagnostic()) == "缺少选项值");

    auto fallback = ErrorFactory::command_disabled("old");
    CHECK(localize(fallback.diagnostic()) == fallback.what());
}
