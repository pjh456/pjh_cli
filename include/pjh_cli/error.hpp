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
    /// ParseResult<T> = Result<T, ParseError>.
    class ParseError
        : public std::runtime_error
    {
    public:
        using std::runtime_error::runtime_error;
    };

    /// @brief Generic parse error at a given argument position.
    inline ParseError
    parse_error(
        std::string_view arg_name,
        int position)
    {
        return ParseError(
            std::format(
                "parse error at argument '{}', "
                "position {}",
                arg_name,
                position));
    }

    /// @brief User specified an option that was not registered.
    inline ParseError
    unknown_option(
        std::string_view name)
    {
        return ParseError(
            std::format(
                "unknown option: '{}'",
                name));
    }

    /// @brief Option declared as taking a value, but none provided.
    inline ParseError
    missing_value(
        std::string_view name)
    {
        return ParseError(
            std::format(
                "option '{}' requires a value",
                name));
    }

    /// @brief Required option was not present on the command line.
    inline ParseError
    missing_required_option(
        std::string_view name)
    {
        return ParseError(
            std::format(
                "missing required option: '{}'",
                name));
    }

    /// @brief Required positional argument was not provided.
    inline ParseError
    missing_required_arg(
        std::string_view name)
    {
        return ParseError(
            std::format(
                "missing required argument: '{}'",
                name));
    }

    /// @brief String value could not be converted to the expected type.
    inline ParseError
    type_conversion_error(
        std::string_view name,
        std::string_view value,
        std::string_view expected_type)
    {
        return ParseError(
            std::format(
                "invalid value '{}' for '{}': "
                "expected {}",
                value, name,
                expected_type));
    }

    /// @brief Multiple commands matched the input (fuzzy match ambiguity).
    inline ParseError
    ambiguous_command(
        std::string_view input,
        const std::vector<std::string> &candidates)
    {
        std::string msg =
            std::format(
                "ambiguous command '{}', "
                "candidates:",
                input);
        for (const auto &c : candidates)
        {
            msg += " " + c;
        }
        return ParseError(msg);
    }

    /// @brief Command exists but is currently disabled via enabled_predicate.
    inline ParseError
    command_disabled(
        std::string_view name)
    {
        return ParseError(
            std::format(
                "command '{}' is not available",
                name));
    }

} // namespace pjh::cli

#endif // INCLUDE_PJH_CLI_ERROR_HPP
