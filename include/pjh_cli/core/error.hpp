#ifndef INCLUDE_PJH_CLI_ERROR_HPP
#define INCLUDE_PJH_CLI_ERROR_HPP

#include <cstddef>
#include <format>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace pjh::cli
{
    /// @brief Error type for parse failures.
    ///
    /// Subclass of std::runtime_error. Used as the error (E) type in
    /// CliResult<T> = Result<T, CliError>.
    /// Every message is automatically prefixed with "Parse Error: ".
    class CliError : public std::runtime_error
    {
    public:
        explicit CliError(const std::string &msg) :
            std::runtime_error(std::string("Parse Error: ") + msg)
        {
        }

        explicit CliError(const char *msg) :
            std::runtime_error(std::string("Parse Error: ") + msg)
        {
        }
    };

    /// @brief Error type for programming mistakes (not parse failures).
    ///
    /// Thrown when the library API is used incorrectly, e.g. calling
    /// ParseContext::get() without checking has() first.
    class LogicError : public std::logic_error
    {
    public:
        using std::logic_error::logic_error;
    };

    /// @brief Utility class for creating CliError instances.
    ///
    /// All methods return a fully-formed CliError with a human-readable
    /// message describing the specific problem.  Errors are returned
    /// by value and should be wrapped in CliFailure or passed directly
    /// to CliResult<T>::Err.
    ///
    /// Usage:
    /// @code
    ///   return CliFailure{ErrorFactory::unknown_option("--foo")};
    /// @endcode
    class ErrorFactory
    {
    public:
        ErrorFactory() = delete;

        /// @brief Generic parse error at a given argument position.
        /// @param arg_name  The argument text that caused the error.
        /// @param position  Zero-based index into the original argument list.
        /// @return CliError with message:
        ///         "Parse Error: parse error at argument '...', position ..."
        static CliError parse_error(std::string_view arg_name, int position)
        {
            return CliError(
                std::format(
                    "parse error at argument '{}', "
                    "position {}",
                    arg_name, position));
        }

        /// @brief User specified an option that was not registered.
        /// @param name  The option as typed, including dashes (e.g. "--bogus").
        /// @return CliError with message:
        ///         "Parse Error: unknown option: '...'"
        static CliError unknown_option(std::string_view name)
        {
            return CliError(std::format("unknown option: '{}'", name));
        }

        /// @brief Option declared as taking a value, but none provided.
        /// @param name  The option name (including dashes).
        /// @return CliError with message:
        ///         "Parse Error: option '...' requires a value"
        static CliError missing_value(std::string_view name)
        {
            return CliError(std::format("option '{}' requires a value", name));
        }

        /// @brief Required option was not present on the command line.
        /// @param name  The long option name (without dashes).
        /// @return CliError with message:
        ///         "Parse Error: missing required option: '...'"
        static CliError missing_required_option(std::string_view name)
        {
            return CliError(std::format("missing required option: '{}'", name));
        }

        /// @brief Required positional argument was not provided.
        /// @param name  The argument display name.
        /// @return CliError with message:
        ///         "Parse Error: missing required argument: '...'"
        static CliError missing_required_arg(std::string_view name)
        {
            return CliError(std::format("missing required argument: '{}'", name));
        }

        /// @brief String value could not be converted to the expected type.
        /// @param name           Option or argument name.
        /// @param value          The raw string that failed conversion.
        /// @param expected_type  Human-readable type name ("integer", "float", etc.).
        /// @return CliError with message:
        ///         "Parse Error: invalid value '...' for '...': expected ..."
        static CliError type_conversion_error(
            std::string_view name, std::string_view value, std::string_view expected_type)
        {
            return CliError(
                std::format(
                    "invalid value '{}' for '{}': "
                    "expected {}",
                    value, name, expected_type));
        }

        /// @brief Multiple commands matched the input (fuzzy match ambiguity).
        /// @param input       The ambiguous user input.
        /// @param candidates  List of candidate command names.
        /// @return CliError listing all candidates.
        static CliError ambiguous_command(
            std::string_view input, const std::vector<std::string> &candidates)
        {
            std::string msg = std::format(
                "ambiguous command '{}', "
                "candidates:",
                input);
            for (const auto &c : candidates)
                msg = std::format("{} {}", std::move(msg), c);
            return CliError(msg);
        }

        /// @brief Value is outside the allowed range [min, max] (int).
        /// @param name   Option name.
        /// @param value  The raw string that was parsed.
        /// @param min    Inclusive lower bound.
        /// @param max    Inclusive upper bound.
        /// @return CliError with the range in the message.
        static CliError value_out_of_range(
            std::string_view name, std::string_view value, int min, int max)
        {
            return CliError(
                std::format(
                    "value '{}' for '{}' is out of range [{}, {}]", value, name, min,
                    max));
        }

        /// @brief Value is outside the allowed range [min, max] (double).
        /// @param name   Option name.
        /// @param value  The raw string that was parsed.
        /// @param min    Inclusive lower bound.
        /// @param max    Inclusive upper bound.
        /// @return CliError with the range in the message.
        static CliError value_out_of_range(
            std::string_view name, std::string_view value, double min, double max)
        {
            return CliError(
                std::format(
                    "value '{}' for '{}' is out of range [{}, {}]", value, name, min,
                    max));
        }

        /// @brief String value did not match any valid enum mapping.
        /// @param raw    The user-supplied (invalid) value.
        /// @param valid  List of valid enum names.
        /// @return CliError with message listing the valid choices.
        static CliError enum_value_error(
            std::string_view raw, const std::vector<std::string> &valid)
        {
            std::string joined;
            for (size_t i = 0; i < valid.size(); i++)
            {
                if (i > 0)
                    joined += ", ";
                joined += valid[i];
            }
            return CliError(
                std::format("invalid value '{}': expected one of: {}", raw, joined));
        }

        /// @brief Command exists but is currently disabled via enabled_predicate.
        /// @param name  The command name.
        /// @return CliError with message:
        ///         "Parse Error: command '...' is not available"
        static CliError command_disabled(std::string_view name)
        {
            return CliError(std::format("command '{}' is not available", name));
        }

        /// @brief Multiple options from a mutually-exclusive group were provided.
        /// @param names  The long option names (e.g. "--port", "--socket").
        /// @return CliError with message listing the conflicting options.
        static CliError conflicting_options(const std::vector<std::string> &names)
        {
            std::string msg;
            for (size_t i = 0; i < names.size(); i++)
            {
                if (i > 0)
                    msg += ", ";
                msg += names[i];
            }
            return CliError(
                std::format("conflicting options: {} cannot be used together", msg));
        }

        /// @brief None of the options in a required group were provided.
        /// @param names       The long option names.
        /// @param exactly_one True for ExactlyOne mode, false for AtLeastOne.
        /// @return CliError with message saying which options are required.
        static CliError required_option_group(
            const std::vector<std::string> &names, bool exactly_one)
        {
            std::string msg;
            for (size_t i = 0; i < names.size(); i++)
            {
                if (i > 0)
                    msg += ", ";
                msg += names[i];
            }
            return CliError(std::format(
                "{} of {} is required",
                exactly_one ? "exactly one" : "at least one",
                msg));
        }
    };

}  // namespace pjh::cli

#endif  // INCLUDE_PJH_CLI_ERROR_HPP
