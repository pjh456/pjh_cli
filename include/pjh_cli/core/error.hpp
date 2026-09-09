#ifndef INCLUDE_PJH_CLI_ERROR_HPP
#define INCLUDE_PJH_CLI_ERROR_HPP

#include <concepts>
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
    ///
    /// `suggestions` is empty unless the producer found close long-option
    /// names worth suggesting.
    struct UnknownOptionError
    {
        std::string option_display;            // "--foo" or "-f"
        std::vector<std::string> suggestions;  ///< Fuzzy candidates, closest first.
    };

    /// @brief Option declared as taking a value, but none provided.
    struct MissingValueError
    {
        std::string option_display;  // "--port" or "-p"
    };

    /// @brief Required option was not present on the command line.
    struct MissingRequiredOptionError
    {
        std::string option_name;  // "--port"
    };

    /// @brief Required positional argument was not provided.
    struct MissingRequiredArgError
    {
        std::string arg_name;  // "file"
    };

    /// @brief String value could not be converted to the expected type.
    struct TypeConversionError
    {
        std::string option_display;  // "--port" or positional arg name ("file")
        std::string raw_value;
        std::string expected_type;  // "integer", "float", "bool (…)"
    };

    /// @brief Multiple commands matched the input (fuzzy match ambiguity).
    struct AmbiguousCommandError
    {
        std::string input;
        std::vector<std::string> candidates;
    };

    /// @brief A word token did not match any subcommand of a dispatcher branch.
    struct UnknownCommandError
    {
        std::string input;
        std::vector<std::string> suggestions;  ///< Fuzzy candidates, closest first.
    };

    /// @brief Value is outside the allowed range [min, max].
    struct ValueOutOfRangeError
    {
        std::string option_display;  // "--port"
        std::string raw_value;
        std::string min;
        std::string max;
    };

    /// @brief String value did not match any valid enum mapping.
    struct EnumValueError
    {
        std::string option_display;  // "--color"
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
        UnknownCommandError,
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

        /// @brief Dependent-false helper for compile-time exhaustive
        ///        `if constexpr` dispatch chains.
        ///
        /// Always `false`, but the value depends on the template parameter, so
        /// a `static_assert(always_false_v<T>)` inside a discarded `else`
        /// branch only fires once that branch is instantiated for an unhandled
        /// type.  Used by format_error() and detail::dispatch_default().
        /// @tparam Ts Ignored; present so the name is usable as a pack.
        template <typename... Ts>
        inline constexpr bool always_false_v = false;
    }

    /// @brief Render an ErrorInfo variant to its human-readable message.
    ///
    /// Exhaustive by construction: the visitor has a final dependent
    /// `static_assert`, so adding an ErrorInfo alternative without a matching
    /// branch is a compile-time error here, not undefined behaviour.
    /// @param info Structured error payload.
    /// @return Rendered message without the "Parse Error: " prefix.
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
                    if (e.suggestions.empty())
                        return std::format("unknown option: '{}'", e.option_display);
                    return std::format(
                        "unknown option: '{}'; did you mean: {}", e.option_display,
                        detail::join(e.suggestions, ", "));
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
                        detail::join(e.suggestions, ", "));
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
                        e.option_display, detail::join(e.valid_choices, ", "));
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
                else
                {
                    static_assert(
                        detail::always_false_v<T>,
                        "unhandled ErrorInfo alternative: add a format_error branch");
                }
            },
            info);
    }

    // ── CliError ─────────────────────────────────────────────────────

    /// @brief Category of a CliError; drives the `what()` prefix.
    enum class ErrorKind
    {
        Parse,   ///< Command-line parsing / validation failure.
        Runtime  ///< Action/execution failure reported by the embedder.
    };

    /// @brief Error type for parse and runtime failures.
    ///
    /// Stores a structured ErrorInfo variant plus an ErrorKind category.
    /// Parse errors (constructed from an ErrorInfo, e.g. via ErrorFactory)
    /// render `what()` as "Parse Error: " + format_error(info).  Runtime
    /// errors (constructed from a plain string, or via
    /// ErrorFactory::runtime_error) render the message only, with no
    /// prefix.  Use kind() to introspect the category instead of parsing
    /// `what()`.
    ///
    /// Note: wrapping a runtime message in ErrorInfo(RawMessageError{...})
    /// and passing it to the one-argument constructor yields a Parse error.
    class CliError : public std::runtime_error
    {
        ErrorInfo m_info;
        ErrorKind m_kind;

        static std::string render_what(const ErrorInfo &info, ErrorKind kind)
        {
            if (kind == ErrorKind::Parse)
                return std::format("Parse Error: {}", format_error(info));
            return format_error(info);
        }

    public:
        /// @brief Construct a parse error from a structured ErrorInfo variant.
        explicit CliError(ErrorInfo info) : CliError(std::move(info), ErrorKind::Parse) {}

        /// @brief Construct an error from a structured ErrorInfo and explicit kind.
        /// @param info Structured error payload.
        /// @param kind Category controlling the `what()` prefix.  Deliberately has
        ///        no default, so the one-argument ErrorInfo overload is unambiguous.
        CliError(ErrorInfo info, ErrorKind kind) :
            std::runtime_error(render_what(info, kind)),
            m_info(std::move(info)),
            m_kind(kind)
        {
        }

        /// @brief Construct a runtime error from a plain string.
        ///
        /// The message is stored as RawMessageError but categorised as
        /// ErrorKind::Runtime, so `what()` returns the message with no
        /// "Parse Error: " prefix.
        explicit CliError(const std::string &msg) :
            CliError(ErrorInfo(RawMessageError{msg}), ErrorKind::Runtime)
        {
        }

        /// @brief Construct a runtime error from a C-string.
        explicit CliError(const char *msg) : CliError(std::string(msg)) {}

        /// @brief Access the structured error information.
        const ErrorInfo &info() const noexcept { return m_info; }

        /// @brief Access the error category.
        ErrorKind kind() const noexcept { return m_kind; }
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

        /// @brief Build an unknown-option error with fuzzy suggestions.
        /// @param display     The option as typed (e.g. "--prot").
        /// @param suggestions  Close candidate displays, closest first
        ///        (e.g. {"--port"}); when empty, renders the plain message.
        /// @return CliError rendering "unknown option: '<display>'; did you
        ///         mean: …" when suggestions are present.
        static CliError unknown_option(
            std::string_view display, const std::vector<std::string> &suggestions)
        {
            return CliError(UnknownOptionError{std::string(display), suggestions});
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

        static CliError unknown_command(
            std::string_view input, const std::vector<std::string> &suggestions)
        {
            return CliError(UnknownCommandError{std::string(input), suggestions});
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
            std::string_view display,
            std::string_view raw,
            const std::vector<std::string> &valid)
        {
            return CliError(
                EnumValueError{std::string(display), std::string(raw), valid});
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

        /// @brief Build a runtime/execution error.  `what()` is the message only.
        static CliError runtime_error(std::string_view message)
        {
            return CliError(
                ErrorInfo(RawMessageError{std::string(message)}), ErrorKind::Runtime);
        }
    };

}  // namespace pjh::cli

#endif  // INCLUDE_PJH_CLI_ERROR_HPP
