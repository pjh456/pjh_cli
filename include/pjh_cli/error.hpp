#ifndef INCLUDE_PJH_CLI_ERROR_HPP
#define INCLUDE_PJH_CLI_ERROR_HPP

#include <format>
#include <stdexcept>
#include <string>
#include <string_view>
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

    /// @brief Generic parse error at a given argument position.
    inline CliError parse_error(std::string_view arg_name, int position)
    {
        return CliError(
            std::format(
                "parse error at argument '{}', "
                "position {}",
                arg_name, position));
    }

    /// @brief User specified an option that was not registered.
    inline CliError unknown_option(std::string_view name)
    {
        return CliError(std::format("unknown option: '{}'", name));
    }

    /// @brief Option declared as taking a value, but none provided.
    inline CliError missing_value(std::string_view name)
    {
        return CliError(std::format("option '{}' requires a value", name));
    }

    /// @brief Required option was not present on the command line.
    inline CliError missing_required_option(std::string_view name)
    {
        return CliError(std::format("missing required option: '{}'", name));
    }

    /// @brief Required positional argument was not provided.
    inline CliError missing_required_arg(std::string_view name)
    {
        return CliError(std::format("missing required argument: '{}'", name));
    }

    /// @brief String value could not be converted to the expected type.
    inline CliError type_conversion_error(
        std::string_view name, std::string_view value, std::string_view expected_type)
    {
        return CliError(
            std::format(
                "invalid value '{}' for '{}': "
                "expected {}",
                value, name, expected_type));
    }

    /// @brief Multiple commands matched the input (fuzzy match ambiguity).
    inline CliError ambiguous_command(
        std::string_view input, const std::vector<std::string> &candidates)
    {
        std::string msg = std::format(
            "ambiguous command '{}', "
            "candidates:",
            input);
        for (const auto &c : candidates) msg = std::format("{} {}", std::move(msg), c);
        return CliError(msg);
    }

    /// @brief Value is not one of the allowed choices.
    inline CliError invalid_choice(
        std::string_view name,
        std::string_view value,
        const std::vector<std::string> &choices)
    {
        std::string msg = std::format(
            "invalid value '{}' for '{}': expected one of: {}", value, name, choices[0]);
        for (size_t i = 1; i < choices.size(); i++)
            msg = std::format("{}, {}", std::move(msg), choices[i]);
        return CliError(msg);
    }

    /// @brief Value is outside the allowed range [min, max].
    inline CliError value_out_of_range(
        std::string_view name, std::string_view value, int min, int max)
    {
        return CliError(
            std::format(
                "value '{}' for '{}' is out of range [{}, {}]", value, name, min, max));
    }

    /// @brief Command exists but is currently disabled via enabled_predicate.
    inline CliError command_disabled(std::string_view name)
    {
        return CliError(std::format("command '{}' is not available", name));
    }

}  // namespace pjh::cli

#endif  // INCLUDE_PJH_CLI_ERROR_HPP
