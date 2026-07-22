#ifndef INCLUDE_PJH_CLI_DETAIL_CONCEPT_HPP
#define INCLUDE_PJH_CLI_DETAIL_CONCEPT_HPP

#include <concepts>
#include <cstddef>
#include <type_traits>

#include <pjh_cli/core/fixed_string.hpp>

namespace pjh::cli::detail
{

    /// @brief Trait: true if T is a valid option/argument key type.
    template <typename T>
    struct is_option_key : std::false_type
    {
    };

    template <std::integral T>
    struct is_option_key<T> : std::true_type
    {
    };

    template <size_t N>
    struct is_option_key<fixed_string<N>> : std::true_type
    {
    };

    /// @brief Concept: valid compile-time key for option/arg access.
    ///
    /// Satisfied by size_t (positional arg index) and fixed_string<N> (named option).
    /// Catches misuses like `ctx.get<int, 3.14>()` at compile time.
    template <typename T>
    concept OptionKey = is_option_key<T>::value;

}  // namespace pjh::cli::detail

#endif
