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
        /// @brief Convert string to integer type using std::from_chars.
        /// @tparam T Integer type (int, long, unsigned, etc.)
        /// @return Ok(T) on success, Err(CliError) on invalid input.
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

        /// @brief Convert string to floating-point type using std::from_chars.
        /// @tparam T Float type (float, double).
        /// @return Ok(T) on success, Err(CliError) on invalid input.
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

    /// @brief Type-to-string converter with explicit specializations.
    ///
    /// Specialize for custom types by providing a from_string static method.
    /// @tparam T Target type to convert from string.
    template <typename T>
    struct Converter;

    template <std::integral T>
    struct Converter<T>
    {
        static auto from_string(std::string_view s) -> CliResult<T>
        {
            return detail::from_chars_int<T>(s);
        }
    };

    template <std::floating_point T>
    struct Converter<T>
    {
        static auto from_string(std::string_view s) -> CliResult<T>
        {
            return detail::from_chars_float<T>(s);
        }
    };

    template <>
    struct Converter<std::string>
    {
        static auto from_string(std::string_view s) -> CliResult<std::string>
        {
            return CliResult<std::string>::Ok(std::string(s));
        }
    };

    template <>
    struct Converter<bool>
    {
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
