#include <cstddef>
#include <format>
#include <pjh_cli/core/error.hpp>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace
{
    std::string join(const std::vector<std::string> &items, std::string_view sep)
    {
        std::string out;
        for (size_t i = 0; i < items.size(); i++)
        {
            if (i > 0)
                out += sep;
            out += items[i];
        }
        return out;
    }
}  // namespace

namespace pjh::cli
{
    std::string format_error(const ErrorInfo &info)
    {
        return std::visit(
            [](const auto &e) -> std::string
            {
                using T = std::decay_t<decltype(e)>;

                if constexpr (std::same_as<T, RawMessageError>)
                {
                    return e.message;
                }
                else if constexpr (std::same_as<T, ParseError>)
                {
                    return std::format(
                        "unexpected argument '{}' at position {}", e.raw_input,
                        e.position);
                }
                else if constexpr (std::same_as<T, UnknownOptionError>)
                {
                    if (e.suggestions.empty())
                        return std::format("unknown option: '{}'", e.option_display);
                    return std::format(
                        "unknown option: '{}'; did you mean: {}", e.option_display,
                        join(e.suggestions, ", "));
                }
                else if constexpr (std::same_as<T, MissingValueError>)
                {
                    return std::format("option '{}' requires a value", e.option_display);
                }
                else if constexpr (std::same_as<T, MissingRequiredOptionError>)
                {
                    return std::format("missing required option: '{}'", e.option_name);
                }
                else if constexpr (std::same_as<T, MissingRequiredArgError>)
                {
                    return std::format("missing required argument: '{}'", e.arg_name);
                }
                else if constexpr (std::same_as<T, TypeConversionError>)
                {
                    return std::format(
                        "invalid value '{}' for '{}': expected {}", e.raw_value,
                        e.option_display, e.expected_type);
                }
                else if constexpr (std::same_as<T, AmbiguousCommandError>)
                {
                    std::string msg =
                        std::format("ambiguous command '{}', candidates:", e.input);
                    for (const auto &c : e.candidates)
                        msg = std::format("{} {}", std::move(msg), c);
                    return msg;
                }
                else if constexpr (std::same_as<T, UnknownCommandError>)
                {
                    if (e.suggestions.empty())
                        return std::format("unknown command: '{}'", e.input);
                    return std::format(
                        "unknown command: '{}'; did you mean: {}", e.input,
                        join(e.suggestions, ", "));
                }
                else if constexpr (std::same_as<T, ValueOutOfRangeError>)
                {
                    return std::format(
                        "value '{}' for '{}' is out of range [{}, {}]", e.raw_value,
                        e.option_display, e.min, e.max);
                }
                else if constexpr (std::same_as<T, EnumValueError>)
                {
                    return std::format(
                        "invalid value '{}' for '{}': expected one of: {}", e.raw_value,
                        e.option_display, join(e.valid_choices, ", "));
                }
                else if constexpr (std::same_as<T, CommandDisabledError>)
                {
                    return std::format("command '{}' is not available", e.command_name);
                }
                else if constexpr (std::same_as<T, ConflictingOptionsError>)
                {
                    return std::format(
                        "conflicting options: {} cannot be used together",
                        join(e.option_names, ", "));
                }
                else if constexpr (std::same_as<T, RequiredOptionGroupError>)
                {
                    return std::format(
                        "{} of {} is required",
                        e.exactly_one ? "exactly one" : "at least one",
                        join(e.option_names, ", "));
                }
                else if constexpr (std::same_as<T, OptionDoesNotAcceptValueError>)
                {
                    return std::format(
                        "option '{}' does not accept a value", e.option_display);
                }
                else if constexpr (std::same_as<T, NoCommandMatchedError>)
                {
                    return std::string("no command matched");
                }
                else
                {
                    static_assert(
                        detail::always_false_v<T>,
                        "unhandled ErrorInfo alternative: add a format_error branch");
                }
            },
            info);
    }

    std::string CliError::render_what(const ErrorInfo &info, ErrorKind kind)
    {
        if (kind == ErrorKind::Parse)
            return std::format("Parse Error: {}", format_error(info));
        return format_error(info);
    }

    ErrorTag CliError::tag() const noexcept
    {
        if (m_kind == ErrorKind::Runtime)
            return ErrorTag::Runtime;

        return std::visit(
            [](const auto &e) -> ErrorTag
            {
                using T = std::decay_t<decltype(e)>;

                if constexpr (std::same_as<T, RawMessageError>)
                    return ErrorTag::RawMessage;
                else if constexpr (std::same_as<T, ParseError>)
                    return ErrorTag::Parse;
                else if constexpr (std::same_as<T, UnknownOptionError>)
                    return ErrorTag::UnknownOption;
                else if constexpr (std::same_as<T, MissingValueError>)
                    return ErrorTag::MissingValue;
                else if constexpr (std::same_as<T, MissingRequiredOptionError>)
                    return ErrorTag::MissingRequiredOption;
                else if constexpr (std::same_as<T, MissingRequiredArgError>)
                    return ErrorTag::MissingRequiredArg;
                else if constexpr (std::same_as<T, TypeConversionError>)
                    return ErrorTag::TypeConversion;
                else if constexpr (std::same_as<T, AmbiguousCommandError>)
                    return ErrorTag::AmbiguousCommand;
                else if constexpr (std::same_as<T, UnknownCommandError>)
                    return ErrorTag::UnknownCommand;
                else if constexpr (std::same_as<T, ValueOutOfRangeError>)
                    return ErrorTag::ValueOutOfRange;
                else if constexpr (std::same_as<T, EnumValueError>)
                    return ErrorTag::EnumValue;
                else if constexpr (std::same_as<T, CommandDisabledError>)
                    return ErrorTag::CommandDisabled;
                else if constexpr (std::same_as<T, ConflictingOptionsError>)
                    return ErrorTag::ConflictingOptions;
                else if constexpr (std::same_as<T, RequiredOptionGroupError>)
                    return ErrorTag::RequiredOptionGroup;
                else if constexpr (std::same_as<T, OptionDoesNotAcceptValueError>)
                    return ErrorTag::OptionDoesNotAcceptValue;
                else if constexpr (std::same_as<T, NoCommandMatchedError>)
                    return ErrorTag::NoCommandMatched;
                else
                {
                    static_assert(
                        detail::always_false_v<T>,
                        "unhandled ErrorInfo alternative: add an ErrorTag mapping");
                }
            },
            m_info);
    }

    ErrorDiagnostic CliError::diagnostic() const noexcept
    {
        return ErrorDiagnostic{*this};
    }

    CliError ErrorFactory::parse_error(std::string_view arg_name, int position)
    {
        return CliError(ParseError{std::string(arg_name), position});
    }

    CliError ErrorFactory::unknown_option(std::string_view display)
    {
        return CliError(UnknownOptionError{std::string(display)});
    }

    CliError ErrorFactory::unknown_option(
        std::string_view display, const std::vector<std::string> &suggestions)
    {
        return CliError(UnknownOptionError{std::string(display), suggestions});
    }

    CliError ErrorFactory::missing_value(std::string_view display)
    {
        return CliError(MissingValueError{std::string(display)});
    }

    CliError ErrorFactory::missing_required_option(std::string_view name)
    {
        return CliError(MissingRequiredOptionError{std::string(name)});
    }

    CliError ErrorFactory::missing_required_arg(std::string_view name)
    {
        return CliError(MissingRequiredArgError{std::string(name)});
    }

    CliError ErrorFactory::type_conversion_error(
        std::string_view name, std::string_view value, std::string_view expected_type)
    {
        return CliError(TypeConversionError{
            std::string(name), std::string(value), std::string(expected_type)});
    }

    CliError ErrorFactory::ambiguous_command(
        std::string_view input, const std::vector<std::string> &candidates)
    {
        return CliError(AmbiguousCommandError{std::string(input), candidates});
    }

    CliError ErrorFactory::unknown_command(
        std::string_view input, const std::vector<std::string> &suggestions)
    {
        return CliError(UnknownCommandError{std::string(input), suggestions});
    }

    CliError ErrorFactory::value_out_of_range(
        std::string_view name, std::string_view value, int min, int max)
    {
        return CliError(ValueOutOfRangeError{
            std::string(name), std::string(value), std::to_string(min),
            std::to_string(max)});
    }

    CliError ErrorFactory::value_out_of_range(
        std::string_view name, std::string_view value, double min, double max)
    {
        return CliError(ValueOutOfRangeError{
            std::string(name), std::string(value), std::format("{}", min),
            std::format("{}", max)});
    }

    CliError ErrorFactory::enum_value_error(
        std::string_view display,
        std::string_view raw,
        const std::vector<std::string> &valid)
    {
        return CliError(EnumValueError{std::string(display), std::string(raw), valid});
    }

    CliError ErrorFactory::command_disabled(std::string_view name)
    {
        return CliError(CommandDisabledError{std::string(name)});
    }

    CliError ErrorFactory::conflicting_options(const std::vector<std::string> &names)
    {
        return CliError(ConflictingOptionsError{names});
    }

    CliError ErrorFactory::required_option_group(
        const std::vector<std::string> &names, bool exactly_one)
    {
        return CliError(RequiredOptionGroupError{names, exactly_one});
    }

    CliError ErrorFactory::option_does_not_accept_value(std::string_view display)
    {
        return CliError(OptionDoesNotAcceptValueError{std::string(display)});
    }

    CliError ErrorFactory::no_command_matched()
    {
        return CliError(NoCommandMatchedError{});
    }

    CliError ErrorFactory::runtime_error(std::string_view message)
    {
        return CliError(
            ErrorInfo(RawMessageError{std::string(message)}), ErrorKind::Runtime);
    }
}  // namespace pjh::cli
