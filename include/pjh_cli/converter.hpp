#ifndef INCLUDE_PJH_CLI_CONVERTER_HPP
#define INCLUDE_PJH_CLI_CONVERTER_HPP

#include <charconv>
#include <concepts>
#include <string>
#include <string_view>
#include <system_error>

#include "detail/string_utils.hpp"
#include "error.hpp"
#include "type.hpp"

namespace pjh::cli
{

    namespace detail
    {
        /// @brief Parse an integer from a string using std::from_chars.
        /// @tparam T Integer type (int, long, unsigned, etc.).
        /// @param s Input string.
        /// @return Ok(T) on success, Err(CliError) if parsing fails or trailing
        ///         characters remain.
        template <std::integral T>
        auto from_chars_int(std::string_view s) -> CliResult<T>
        {
            T v{};
            auto [ptr, ec] = std::from_chars(s.data(), s.data() + s.size(), v);
            if (ec == std::errc() && ptr == s.data() + s.size())
                return CliResult<T>::Ok(v);
            return CliResult<T>::Err(
                CliError("invalid integer: '" + std::string(s) + "'"));
        }

        /// @brief Parse a floating-point number from a string using std::from_chars.
        /// @tparam T Float type (float, double).
        /// @param s Input string.
        /// @return Ok(T) on success, Err(CliError) if parsing fails.
        template <std::floating_point T>
        auto from_chars_float(std::string_view s) -> CliResult<T>
        {
            T v{};
            auto [ptr, ec] = std::from_chars(s.data(), s.data() + s.size(), v);
            if (ec == std::errc() && ptr == s.data() + s.size())
                return CliResult<T>::Ok(v);
            return CliResult<T>::Err(
                CliError("invalid number: '" + std::string(s) + "'"));
        }

    }  // namespace detail

    /// @brief String-to-type converter with explicit specializations.
    ///
    /// Specialise for custom types by providing a `static from_string` method
    /// returning CliResult<T>.
    ///
    /// Builtin specializations:
    ///   - int / long / unsigned / … via std::from_chars
    ///   - float / double via std::from_chars
    ///   - std::string (identity copy)
    ///   - bool (recognises true/false/yes/no/1/0, case-insensitive)
    ///
    /// @tparam T Target type.
    template <typename T>
    struct Converter;

    /// @brief Integral types (int, long, unsigned, …).
    template <std::integral T>
    struct Converter<T>
    {
        /// @brief Parse @p s as an integer.
        /// @param s Raw input string.
        /// @return Ok(T) or Err(CliError) on invalid input.
        static auto from_string(std::string_view s) -> CliResult<T>
        {
            return detail::from_chars_int<T>(s);
        }
    };

    /// @brief Floating-point types (float, double).
    template <std::floating_point T>
    struct Converter<T>
    {
        /// @brief Parse @p s as a floating-point number.
        /// @param s Raw input string.
        /// @return Ok(T) or Err(CliError) on invalid input.
        static auto from_string(std::string_view s) -> CliResult<T>
        {
            return detail::from_chars_float<T>(s);
        }
    };

    /// @brief Trivially copies the input string.
    template <>
    struct Converter<std::string>
    {
        /// @brief Return a copy of @p s.
        /// @param s Raw input string.
        /// @return Ok(s) — always succeeds.
        static auto from_string(std::string_view s) -> CliResult<std::string>
        {
            return CliResult<std::string>::Ok(std::string(s));
        }
    };

    /// @brief Case-insensitive bool parsing.
    ///
    /// Accepted true values: "true", "1", "yes", "y"
    /// Accepted false values: "false", "0", "no", "n"
    template <>
    struct Converter<bool>
    {
        /// @brief Parse @p s as a boolean.
        /// @param s Raw input string.
        /// @return Ok(true/false) or Err(CliError) if input is not recognised.
        static auto from_string(std::string_view s) -> CliResult<bool>
        {
            if (detail::StringUtils::case_insensitive_equal(s, "true") ||
                detail::StringUtils::case_insensitive_equal(s, "1") ||
                detail::StringUtils::case_insensitive_equal(s, "yes") ||
                detail::StringUtils::case_insensitive_equal(s, "y"))
                return CliResult<bool>::Ok(true);
            if (detail::StringUtils::case_insensitive_equal(s, "false") ||
                detail::StringUtils::case_insensitive_equal(s, "0") ||
                detail::StringUtils::case_insensitive_equal(s, "no") ||
                detail::StringUtils::case_insensitive_equal(s, "n"))
                return CliResult<bool>::Ok(false);
            return CliResult<bool>::Err(
                CliError{
                    "invalid bool: '" + std::string(s) +
                    "', expected true/false/yes/no/1/0"});
        }
    };

}  // namespace pjh::cli

#endif  // INCLUDE_PJH_CLI_CONVERTER_HPP
