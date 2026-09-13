#ifndef INCLUDE_PJH_CLI_CONVERTER_HPP
#define INCLUDE_PJH_CLI_CONVERTER_HPP

#include <charconv>
#include <cmath>
#include <concepts>
#include <filesystem>
#include <pjh_cli/core/error.hpp>
#include <pjh_cli/core/type.hpp>
#include <pjh_cli/detail/string_utils.hpp>
#include <string>
#include <string_view>
#include <system_error>
#include <type_traits>

namespace pjh::cli
{

    namespace detail
    {
        /// @brief Stable expected-type tag for a Converter<T> target.
        /// @tparam T Target type.
        /// @return Bool for bool (checked before integral), Float for
        ///         floating-point types, Integer otherwise.
        template <typename T>
        constexpr ExpectedType expected_type_tag() noexcept
        {
            if constexpr (std::is_same_v<T, bool>)  // must precede integral
                return ExpectedType::Bool;
            else if constexpr (std::floating_point<T>)
                return ExpectedType::Float;
            else
                return ExpectedType::Integer;
        }

        /// @brief Human-readable expected type name for TypeConversionError.
        /// @tparam T Target type.
        /// @return "bool (true/false/yes/no/1/0)" for bool, "float" for
        ///         floating-point types, "integer" otherwise.
        template <typename T>
        constexpr std::string_view expected_type_name() noexcept
        {
            return pjh::cli::expected_type_name(expected_type_tag<T>());
        }

        /// @brief Strip one optional leading '+' from @p s.
        ///
        /// std::from_chars recognizes only a leading '-' (not '+') for integers
        /// and floating-point values.  This accepts the human-friendly '+N' form
        /// by removing a single leading '+'.  Returns false for a lone '+', a
        /// doubled sign ("+-5", "++5"), or '+' followed by end-of-input, which
        /// the caller must reject.
        ///
        /// @param s In/out view; the leading '+' is removed on success.
        /// @return true when the caller may continue parsing, false to reject.
        inline bool strip_leading_plus(std::string_view &s) noexcept
        {
            if (s.empty() || s.front() != '+')
                return true;
            s.remove_prefix(1);
            return !s.empty() && s.front() != '+' && s.front() != '-';
        }

        /// @brief Parse an integer from a string using std::from_chars.
        /// @tparam T Integer type (int, long, unsigned, etc.).
        /// @param s Input string; one optional leading '+' is accepted.
        /// @param display Option display or arg name used in the error message
        ///        (e.g. "--port" or "file"); empty renders `for ''`.
        /// @return Ok(T) on success, Err(CliError) if parsing fails or trailing
        ///         characters remain.  A rejected leading '+' is still reported
        ///         verbatim in the error's raw value.
        template <std::integral T>
        auto from_chars_int(std::string_view s, std::string_view display = {})
            -> CliResult<T>
        {
            const std::string_view raw = s;
            T v{};
            if (strip_leading_plus(s))
            {
                auto [ptr, ec] = std::from_chars(s.data(), s.data() + s.size(), v);
                if (ec == std::errc() && ptr == s.data() + s.size())
                    return CliResult<T>::Ok(v);
            }
            return CliResult<T>::Err(ErrorFactory::type_conversion_error(
                display, raw, expected_type_tag<T>()));
        }

        /// @brief Parse a floating-point number from a string using std::from_chars.
        /// @tparam T Float type (float, double).
        /// @param s Input string; one optional leading '+' is accepted.
        /// @param display Option display or arg name used in the error message
        ///        (e.g. "--rate" or "ratio"); empty renders `for ''`.
        /// @return Ok(T) on success for a finite value, Err(CliError) otherwise.
        ///         Non-finite tokens (nan, inf, infinity, nan(...), signed and
        ///         case-insensitive) are rejected as invalid values; the error's
        ///         raw value preserves any leading '+'.
        template <std::floating_point T>
        auto from_chars_float(std::string_view s, std::string_view display = {})
            -> CliResult<T>
        {
            const std::string_view raw = s;
            T v{};
            if (strip_leading_plus(s))
            {
                auto [ptr, ec] = std::from_chars(s.data(), s.data() + s.size(), v);
                if (ec == std::errc() && ptr == s.data() + s.size() && std::isfinite(v))
                    return CliResult<T>::Ok(v);
            }
            return CliResult<T>::Err(ErrorFactory::type_conversion_error(
                display, raw, expected_type_tag<T>()));
        }

    }  // namespace detail

    /// @brief String-to-type converter with explicit specializations.
    ///
    /// Specialise for custom types by providing a `static from_string` method
    /// returning CliResult<T>.
    ///
    /// Builtin specializations:
    ///   - int / long / unsigned / … via std::from_chars, accepting one
    ///     optional leading '+' or '-'
    ///   - float / double via std::from_chars, accepting one optional leading
    ///     '+' or '-'; the value must be finite (nan/inf/infinity rejected as
    ///     invalid values via TypeConversionError)
    ///   - std::string (identity copy)
    ///   - std::filesystem::path (trivial construction)
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
        /// @param s Raw input string; one optional leading '+' is accepted.
        /// @param display Option display or arg name used in the error message;
        ///        empty renders `for ''` (direct callers only).
        /// @return Ok(T) or Err(CliError) on invalid input.
        static auto from_string(std::string_view s, std::string_view display = {})
            -> CliResult<T>
        {
            return detail::from_chars_int<T>(s, display);
        }
    };

    /// @brief Floating-point types (float, double).
    template <std::floating_point T>
    struct Converter<T>
    {
        /// @brief Parse @p s as a floating-point number.
        /// @param s Raw input string; one optional leading '+' is accepted.
        /// @param display Option display or arg name used in the error message;
        ///        empty renders `for ''` (direct callers only).
        /// @return Ok(T) for a finite value, Err(CliError) otherwise (including
        ///         non-finite nan/inf/infinity tokens).
        static auto from_string(std::string_view s, std::string_view display = {})
            -> CliResult<T>
        {
            return detail::from_chars_float<T>(s, display);
        }
    };

    /// @brief Trivially copies the input string.
    template <>
    struct Converter<std::string>
    {
        /// @brief Return a copy of @p s.
        /// @param s Raw input string.
        /// @param display Option display or arg name; ignored (kept for a
        ///        uniform call shape with the other converters).
        /// @return Ok(s) — always succeeds.
        static auto from_string(std::string_view s, std::string_view display = {})
            -> CliResult<std::string>
        {
            (void)display;
            return CliResult<std::string>::Ok(std::string(s));
        }
    };

    /// @brief Trivially constructs a path from the input string.
    template <>
    struct Converter<std::filesystem::path>
    {
        /// @brief Construct a path from @p s.
        /// @param s Raw input string.
        /// @param display Option display or arg name; ignored (kept for a
        ///        uniform call shape with the other converters).
        /// @return Ok(std::filesystem::path(s)) — always succeeds.
        static auto from_string(std::string_view s, std::string_view display = {})
            -> CliResult<std::filesystem::path>
        {
            (void)display;
            return CliResult<std::filesystem::path>::Ok(std::filesystem::path(s));
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
        /// @param display Option display or arg name used in the error message;
        ///        empty renders `for ''` (direct callers only).
        /// @return Ok(true/false) or Err(CliError) if input is not recognised.
        static auto from_string(std::string_view s, std::string_view display = {})
            -> CliResult<bool>
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
            return CliResult<bool>::Err(ErrorFactory::type_conversion_error(
                display, s, detail::expected_type_tag<bool>()));
        }
    };

}  // namespace pjh::cli

#endif  // INCLUDE_PJH_CLI_CONVERTER_HPP
