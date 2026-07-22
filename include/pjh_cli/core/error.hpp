#ifndef INCLUDE_PJH_CLI_ERROR_HPP
#define INCLUDE_PJH_CLI_ERROR_HPP

#include <cstddef>
#include <format>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

namespace pjh::cli
{

    // ── Error detail structs ─────────────────────────────────────────

    /// @brief Raw message with no structure. Escape hatch for simple errors.
    struct RawMessageError
    {
        std::string message;
    };

    /// @brief Generic parse error at a specific argument position.
    struct ParseError
    {
        std::string raw_input;
        int position;
    };

    /// @brief User specified an option that was not registered.
    struct UnknownOptionError
    {
        std::string option_display;  // "--foo" or "-f"
    };

    /// @brief Option declared as taking a value, but none provided.
    struct MissingValueError
    {
        std::string option_display;  // "--port" or "-p"
    };

    /// @brief Required option was not present on the command line.
    struct MissingRequiredOptionError
    {
        std::string option_name;  // "port" (without dashes)
    };

    /// @brief Required positional argument was not provided.
    struct MissingRequiredArgError
    {
        std::string arg_name;  // "file"
    };

    /// @brief String value could not be converted to the expected type.
    struct TypeConversionError
    {
        std::string option_display;  // "--port" or arg name
        std::string raw_value;
        std::string expected_type;  // "integer", "float", "bool"
    };

    /// @brief Multiple commands matched the input (fuzzy match ambiguity).
    struct AmbiguousCommandError
    {
        std::string input;
        std::vector<std::string> candidates;
    };

    /// @brief Value is outside the allowed range [min, max].
    struct ValueOutOfRangeError
    {
        std::string option_display;
        std::string raw_value;
        std::string min;
        std::string max;
    };

    /// @brief String value did not match any valid enum mapping.
    struct EnumValueError
    {
        std::string raw_value;
        std::vector<std::string> valid_choices;
    };

    /// @brief Command exists but is currently disabled.
    struct CommandDisabledError
    {
        std::string command_name;
    };

    /// @brief Multiple options from a mutually-exclusive group were provided.
    struct ConflictingOptionsError
    {
        std::vector<std::string> option_names;
    };

    /// @brief A required option group was not satisfied.
    struct RequiredOptionGroupError
    {
        std::vector<std::string> option_names;
        bool exactly_one;  // true=ExactlyOne, false=AtLeastOne
    };

    /// @brief Attempted to pass a value to a flag-style option.
    struct OptionDoesNotAcceptValueError
    {
        std::string option_display;
    };

    /// @brief The parser finished but no command was matched.
    struct NoCommandMatchedError
    {
    };

    // ── ErrorInfo variant ────────────────────────────────────────────

    /// @brief Type-safe variant covering all possible parse errors.
    using ErrorInfo = std::variant<
        RawMessageError,
        ParseError,
        UnknownOptionError,
        MissingValueError,
        MissingRequiredOptionError,
        MissingRequiredArgError,
        TypeConversionError,
        AmbiguousCommandError,
        ValueOutOfRangeError,
        EnumValueError,
        CommandDisabledError,
        ConflictingOptionsError,
        RequiredOptionGroupError,
        OptionDoesNotAcceptValueError,
        NoCommandMatchedError>;

    // ── format_error — centralised ───────────────────────────────────

    namespace detail
    {
        inline std::string join(
            const std::vector<std::string> &items, std::string_view sep)
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
    }

    inline std::string format_error(const ErrorInfo &info)
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
                        "parse error at argument '{}', position {}", e.raw_input,
                        e.position);
                }
                else if constexpr (std::same_as<T, UnknownOptionError>)
                {
                    return std::format("unknown option: '{}'", e.option_display);
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
                else if constexpr (std::same_as<T, ValueOutOfRangeError>)
                {
                    return std::format(
                        "value '{}' for '{}' is out of range [{}, {}]", e.raw_value,
                        e.option_display, e.min, e.max);
                }
                else if constexpr (std::same_as<T, EnumValueError>)
                {
                    return std::format(
                        "invalid value '{}': expected one of: {}", e.raw_value,
                        detail::join(e.valid_choices, ", "));
                }
                else if constexpr (std::same_as<T, CommandDisabledError>)
                {
                    return std::format("command '{}' is not available", e.command_name);
                }
                else if constexpr (std::same_as<T, ConflictingOptionsError>)
                {
                    return std::format(
                        "conflicting options: {} cannot be used together",
                        detail::join(e.option_names, ", "));
                }
                else if constexpr (std::same_as<T, RequiredOptionGroupError>)
                {
                    return std::format(
                        "{} of {} is required",
                        e.exactly_one ? "exactly one" : "at least one",
                        detail::join(e.option_names, ", "));
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
            },
            info);
    }

    // ── CliError ─────────────────────────────────────────────────────

    /// @brief Error type for parse failures.
    ///
    /// Stores a structured ErrorInfo variant.  The `what()` string is
    /// automatically prefixed with "Parse Error: " and built from the
    /// variant via format_error().
    class CliError : public std::runtime_error
    {
        ErrorInfo m_info;

    public:
        /// @brief Construct from a structured ErrorInfo variant.
        explicit CliError(ErrorInfo info) :
            std::runtime_error(std::format("Parse Error: {}", format_error(info))),
            m_info(std::move(info))
        {
        }

        /// @brief Construct from a plain string (wraps in RawMessageError).
        explicit CliError(const std::string &msg) :
            CliError(ErrorInfo(RawMessageError{msg}))
        {
        }

        /// @brief Construct from a C-string.
        explicit CliError(const char *msg) : CliError(std::string(msg)) {}

        /// @brief Access the structured error information.
        const ErrorInfo &info() const noexcept { return m_info; }
    };

    // ── LogicError ───────────────────────────────────────────────────

    /// @brief Error type for programming mistakes (not parse failures).
    ///
    /// Thrown when the library API is used incorrectly, e.g. calling
    /// ParseContext::get() without checking has() first.
    class LogicError : public std::logic_error
    {
    public:
        using std::logic_error::logic_error;
    };

    // ── ErrorFactory ─────────────────────────────────────────────────

    /// @brief Utility class for creating CliError instances.
    ///
    /// All methods return a fully-formed CliError storing a structured
    /// ErrorInfo variant.  The human-readable `what()` string matches
    /// the variant content.
    ///
    /// Usage:
    /// @code
    ///   return CliFailure{ErrorFactory::unknown_option("--foo")};
    /// @endcode
    class ErrorFactory
    {
    public:
        ErrorFactory() = delete;

        static CliError parse_error(std::string_view arg_name, int position)
        {
            return CliError(ParseError{std::string(arg_name), position});
        }

        static CliError unknown_option(std::string_view display)
        {
            return CliError(UnknownOptionError{std::string(display)});
        }

        static CliError missing_value(std::string_view display)
        {
            return CliError(MissingValueError{std::string(display)});
        }

        static CliError missing_required_option(std::string_view name)
        {
            return CliError(MissingRequiredOptionError{std::string(name)});
        }

        static CliError missing_required_arg(std::string_view name)
        {
            return CliError(MissingRequiredArgError{std::string(name)});
        }

        static CliError type_conversion_error(
            std::string_view name, std::string_view value, std::string_view expected_type)
        {
            return CliError(
                TypeConversionError{
                    std::string(name), std::string(value), std::string(expected_type)});
        }

        static CliError ambiguous_command(
            std::string_view input, const std::vector<std::string> &candidates)
        {
            return CliError(AmbiguousCommandError{std::string(input), candidates});
        }

        static CliError value_out_of_range(
            std::string_view name, std::string_view value, int min, int max)
        {
            return CliError(
                ValueOutOfRangeError{
                    std::string(name), std::string(value), std::to_string(min),
                    std::to_string(max)});
        }

        static CliError value_out_of_range(
            std::string_view name, std::string_view value, double min, double max)
        {
            return CliError(
                ValueOutOfRangeError{
                    std::string(name), std::string(value), std::format("{}", min),
                    std::format("{}", max)});
        }

        static CliError enum_value_error(
            std::string_view raw, const std::vector<std::string> &valid)
        {
            return CliError(EnumValueError{std::string(raw), valid});
        }

        static CliError command_disabled(std::string_view name)
        {
            return CliError(CommandDisabledError{std::string(name)});
        }

        static CliError conflicting_options(const std::vector<std::string> &names)
        {
            return CliError(ConflictingOptionsError{names});
        }

        static CliError required_option_group(
            const std::vector<std::string> &names, bool exactly_one)
        {
            return CliError(RequiredOptionGroupError{names, exactly_one});
        }

        static CliError option_does_not_accept_value(std::string_view display)
        {
            return CliError(OptionDoesNotAcceptValueError{std::string(display)});
        }

        static CliError no_command_matched() { return CliError(NoCommandMatchedError{}); }
    };

}  // namespace pjh::cli

#endif  // INCLUDE_PJH_CLI_ERROR_HPP
