#ifndef INCLUDE_PJH_CLI_CONVERTER_HPP
#define INCLUDE_PJH_CLI_CONVERTER_HPP

#include "error.hpp"
#include "type.hpp"

#include "detail/string_utils.hpp"

#include <charconv>
#include <concepts>
#include <string>
#include <string_view>

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

    } // namespace detail

    /// @brief Type-to-string converter with explicit specializations.
    ///
    /// Specialize for custom types by providing a from_string static method.
    /// @tparam T Target type to convert from string.
    template <typename T>
    struct Converter
    {
        /// @brief Parse string as type T.
        /// @return Ok(T) on success, Err(CliError) on failure.
        static auto from_string(std::string_view s) -> CliResult<T>;
    };

    /// @cond CONVERTER_SPECIALIZATIONS

    template <>
    inline auto Converter<int>::from_string(std::string_view s) -> CliResult<int>
    {
        return detail::from_chars_int<int>(s);
    }

    template <>
    inline auto Converter<long>::from_string(std::string_view s) -> CliResult<long>
    {
        return detail::from_chars_int<long>(s);
    }

    template <>
    inline auto Converter<long long>::from_string(std::string_view s) -> CliResult<long long>
    {
        return detail::from_chars_int<long long>(s);
    }

    template <>
    inline auto Converter<unsigned int>::from_string(std::string_view s)
        -> CliResult<unsigned int>
    {
        return detail::from_chars_int<unsigned int>(s);
    }

    template <>
    inline auto Converter<unsigned long>::from_string(std::string_view s)
        -> CliResult<unsigned long>
    {
        return detail::from_chars_int<unsigned long>(s);
    }

    template <>
    inline auto Converter<unsigned long long>::from_string(std::string_view s)
        -> CliResult<unsigned long long>
    {
        return detail::from_chars_int<unsigned long long>(s);
    }

    template <>
    inline auto Converter<float>::from_string(std::string_view s) -> CliResult<float>
    {
        return detail::from_chars_float<float>(s);
    }

    template <>
    inline auto Converter<double>::from_string(std::string_view s) -> CliResult<double>
    {
        return detail::from_chars_float<double>(s);
    }

    template <>
    inline auto Converter<std::string>::from_string(std::string_view s)
        -> CliResult<std::string>
    {
        return CliResult<std::string>::Ok(std::string(s));
    }

    /// @brief Case-insensitive bool parser.
    ///
    /// Accepted values: true/false, yes/no, 1/0, y/n (case-insensitive).
    template <>
    inline auto Converter<bool>::from_string(std::string_view s) -> CliResult<bool>
    {
        if (detail::case_insensitive_equal(s, "true") ||
            detail::case_insensitive_equal(s, "1") ||
            detail::case_insensitive_equal(s, "yes") ||
            detail::case_insensitive_equal(s, "y"))
            return CliResult<bool>::Ok(true);
        if (detail::case_insensitive_equal(s, "false") ||
            detail::case_insensitive_equal(s, "0") ||
            detail::case_insensitive_equal(s, "no") ||
            detail::case_insensitive_equal(s, "n"))
            return CliResult<bool>::Ok(false);
        return CliResult<bool>::Err(CliError{
            "invalid bool: '" +
            std::string(s) +
            "', expected true/false/yes/no/1/0"});
    }

    /// @endcond

} // namespace pjh::cli

#endif // INCLUDE_PJH_CLI_CONVERTER_HPP
