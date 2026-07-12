#ifndef INCLUDE_PJH_CLI_FIXED_STRING_HPP
#define INCLUDE_PJH_CLI_FIXED_STRING_HPP

#include <algorithm>
#include <string_view>

namespace pjh::cli
{
    /// @brief Compile-time fixed-length string for non-type template parameters.
    ///
    /// Allows string literals to be used as template arguments (C++20 NTTP).
    /// All data members are public to satisfy structural type requirements.
    /// @tparam N String length including null terminator.
    template <size_t N>
    struct fixed_string
    {
        char value[N]{};

        /// @brief Construct from a string literal.
        constexpr fixed_string(
            const char (&str)[N]) noexcept
        {
            std::copy_n(str, N, value);
        }

        /// @brief Return as string_view (excluding null terminator).
        constexpr std::string_view
        view() const noexcept
        {
            return std::string_view(value, N - 1);
        }

        /// @brief String length (excluding null terminator).
        constexpr size_t size()
            const noexcept { return N - 1; }

        /// @brief Equality comparison.
        constexpr bool operator==(
            const fixed_string &rhs)
            const noexcept
        {
            return view() == rhs.view();
        }
    };

    /// @brief CTAD deduction guide for fixed_string.
    template <size_t N>
    fixed_string(const char (&)[N])
        -> fixed_string<N>;

} // namespace pjh::cli

#endif // INCLUDE_PJH_CLI_FIXED_STRING_HPP
