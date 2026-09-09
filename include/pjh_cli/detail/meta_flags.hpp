#ifndef INCLUDE_PJH_CLI_DETAIL_META_FLAGS_HPP
#define INCLUDE_PJH_CLI_DETAIL_META_FLAGS_HPP

#include <string_view>

namespace pjh::cli::detail
{
    /// @brief True if @p tok is a token the parser reserves for help
    ///        ("--help" or "-h").
    constexpr bool is_meta_help_token(std::string_view tok) noexcept
    {
        return tok == "--help" || tok == "-h";
    }

    /// @brief True if @p tok is the token the parser reserves for version
    ///        ("--version").
    constexpr bool is_meta_version_token(std::string_view tok) noexcept
    {
        return tok == "--version";
    }

    /// @brief True if @p name (without the "--" prefix) is a reserved long
    ///        option name.
    constexpr bool is_reserved_long_name(std::string_view name) noexcept
    {
        return name == "help" || name == "version";
    }

    /// @brief True if @p c is the reserved short option character.
    constexpr bool is_reserved_short_name(char c) noexcept { return c == 'h'; }
}  // namespace pjh::cli::detail

#endif  // INCLUDE_PJH_CLI_DETAIL_META_FLAGS_HPP
