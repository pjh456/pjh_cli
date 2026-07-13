#ifndef INCLUDE_PJH_CLI_DETAIL_CONCEPT_HPP
#define INCLUDE_PJH_CLI_DETAIL_CONCEPT_HPP

#include "../converter.hpp"
#include "../fixed_string.hpp"
#include "../type.hpp"

#include <concepts>
#include <type_traits>

namespace pjh::cli::detail
{

    /// @brief Trait: true if T has a valid Converter<T> specialization
    ///        returning CliResult<T>.
    template <typename T, typename = void>
    struct is_parseable : std::false_type {};

    /// @brief Specialization matched when Converter<T>::from_string exists
    ///        and returns CliResult<T>.
    template <typename T>
    struct is_parseable<
        T,
        std::void_t<
            decltype(Converter<T>::from_string(
                std::declval<std::string_view>()))>>
        : std::is_same<
              decltype(Converter<T>::from_string(
                  std::declval<std::string_view>())),
              CliResult<T>>
    {
    };

    /// @brief Variable template for is_parseable.
    template <typename T>
    inline constexpr bool is_parseable_v =
        is_parseable<T>::value;

    /// @brief Concept: T can be parsed from string by the library.
    ///
    /// Satisfied when a Converter<T> specialization with a valid
    /// from_string method returning CliResult<T> exists.
    /// @tparam T Candidate type.
    template <typename T>
    concept Parseable = is_parseable_v<T>;

    /// @brief Concept: T is a flag type (bool).
    ///
    /// Flag options do not consume a value token from the command line.
    /// @tparam T Candidate type.
    template <typename T>
    concept Flag = std::is_same_v<T, bool>;

    /// @brief Trait: true if T is a valid option/argument key type.
    template <typename T>
    struct is_option_key : std::false_type {};

    template <std::integral T>
    struct is_option_key<T> : std::true_type {};

    template <size_t N>
    struct is_option_key<fixed_string<N>> : std::true_type {};

    /// @brief Concept: valid compile-time key for option/arg access.
    ///
    /// Satisfied by size_t (positional arg index) and fixed_string<N> (named option).
    /// Catches misuses like `ctx.get<int, 3.14>()` at compile time.
    template <typename T>
    concept OptionKey = is_option_key<T>::value;

} // namespace pjh::cli::detail

#endif
