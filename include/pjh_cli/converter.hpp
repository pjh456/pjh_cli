#ifndef INCLUDE_PJH_CLI_CONVERTER_HPP
#define INCLUDE_PJH_CLI_CONVERTER_HPP

#include "error.hpp"
#include "type.hpp"

#include "detail/string_utils.hpp"

#include <charconv>
#include <string>
#include <string_view>

namespace pjh::cli
{

    namespace detail
    {

        /// @brief Convert string to integer type using std::from_chars.
        /// @tparam T Integer type (int, long, unsigned, etc.)
        /// @return Ok(T) on success, Err(ParseError) on invalid input.
        template <typename T>
        auto from_chars_int(std::string_view s) -> ParseResult<T>
        {
            T v{};
            auto [ptr, ec] = std::from_chars(s.data(), s.data() + s.size(), v);
            if (ec == std::errc() && ptr == s.data() + s.size())
                return ParseResult<T>::Ok(v);
            return ParseResult<T>::Err(
                ParseError("invalid integer: '" + std::string(s) + "'"));
        }

        /// @brief Convert string to floating-point type using std::from_chars.
        /// @tparam T Float type (float, double).
        /// @return Ok(T) on success, Err(ParseError) on invalid input.
        template <typename T>
        auto from_chars_float(std::string_view s) -> ParseResult<T>
        {
            T v{};
            auto [ptr, ec] = std::from_chars(s.data(), s.data() + s.size(), v);
            if (ec == std::errc() && ptr == s.data() + s.size())
                return ParseResult<T>::Ok(v);
            return ParseResult<T>::Err(
                ParseError("invalid number: '" + std::string(s) + "'"));
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
        /// @return Ok(T) on success, Err(ParseError) on failure.
        static auto from_string(std::string_view s) -> ParseResult<T>;
    };

    /// @cond CONVERTER_SPECIALIZATIONS

    template <>
    inline auto Converter<int>::from_string(std::string_view s) -> ParseResult<int>
    {
        return detail::from_chars_int<int>(s);
    }

    template <>
    inline auto Converter<long>::from_string(std::string_view s) -> ParseResult<long>
    {
        return detail::from_chars_int<long>(s);
    }

    template <>
    inline auto Converter<long long>::from_string(std::string_view s) -> ParseResult<long long>
    {
        return detail::from_chars_int<long long>(s);
    }

    template <>
    inline auto Converter<unsigned int>::from_string(std::string_view s)
        -> ParseResult<unsigned int>
    {
        return detail::from_chars_int<unsigned int>(s);
    }

    template <>
    inline auto Converter<unsigned long>::from_string(std::string_view s)
        -> ParseResult<unsigned long>
    {
        return detail::from_chars_int<unsigned long>(s);
    }

    template <>
    inline auto Converter<unsigned long long>::from_string(std::string_view s)
        -> ParseResult<unsigned long long>
    {
        return detail::from_chars_int<unsigned long long>(s);
    }

    template <>
    inline auto Converter<float>::from_string(std::string_view s) -> ParseResult<float>
    {
        return detail::from_chars_float<float>(s);
    }

    template <>
    inline auto Converter<double>::from_string(std::string_view s) -> ParseResult<double>
    {
        return detail::from_chars_float<double>(s);
    }

    template <>
    inline auto Converter<std::string>::from_string(std::string_view s)
        -> ParseResult<std::string>
    {
        return ParseResult<std::string>::Ok(std::string(s));
    }

    /// @brief Case-insensitive bool parser.
    ///
    /// Accepted values: true/false, yes/no, 1/0, y/n (case-insensitive).
    template <>
    inline auto Converter<bool>::from_string(std::string_view s) -> ParseResult<bool>
    {
        if (detail::case_insensitive_equal(s, "true") ||
            detail::case_insensitive_equal(s, "1") ||
            detail::case_insensitive_equal(s, "yes") ||
            detail::case_insensitive_equal(s, "y"))
            return ParseResult<bool>::Ok(true);
        if (detail::case_insensitive_equal(s, "false") ||
            detail::case_insensitive_equal(s, "0") ||
            detail::case_insensitive_equal(s, "no") ||
            detail::case_insensitive_equal(s, "n"))
            return ParseResult<bool>::Ok(false);
        return ParseResult<bool>::Err(ParseError{
            "invalid bool: '" +
            std::string(s) +
            "', expected true/false/yes/no/1/0"});
    }

    /// @endcond

} // namespace pjh::cli

#endif // INCLUDE_PJH_CLI_CONVERTER_HPP
