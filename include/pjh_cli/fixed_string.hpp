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
        template <size_t M>
        constexpr bool operator==(
            const fixed_string<M> &rhs)
            const noexcept
        {
            return view() == rhs.view();
        }
    };

    /// @brief CTAD deduction guide for fixed_string.
    template <size_t N>
    fixed_string(const char (&)[N])
        -> fixed_string<N>;

    /// @brief Hash a size_t key (identity — positional arg index is its own hash).
    constexpr size_t
    key_hash(size_t k) noexcept
    {
        return k;
    }

    /// @brief Hash a fixed_string key (FNV-1a).
    /// @tparam N String length including null terminator.
    template <size_t N>
    constexpr size_t
    key_hash(
        const fixed_string<N> &s) noexcept
    {
        size_t h = 14695981039346656037ULL;
        for (auto c : s.view())
        {
            h ^= static_cast<size_t>(c);
            h *= 1099511628211ULL;
        }
        return h;
    }

} // namespace pjh::cli

#endif // INCLUDE_PJH_CLI_FIXED_STRING_HPP
