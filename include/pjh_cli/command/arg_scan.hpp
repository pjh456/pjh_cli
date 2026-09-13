#ifndef INCLUDE_PJH_CLI_COMMAND_ARG_SCAN_HPP
#define INCLUDE_PJH_CLI_COMMAND_ARG_SCAN_HPP

#include <cstddef>
#include <pjh_cli/command/base_command.hpp>
#include <pjh_cli/detail/tokenizer.hpp>
#include <string_view>

namespace pjh::cli::detail
{

    /// @brief Result of interpreting one dash-prefixed token as an option.
    struct OptionTokenScan
    {
        const OptionDef *option = nullptr;  ///< Resolved option (null if unknown).
        bool needs_next_token = false;      ///< Value is the following token.
        bool compact_value = false;  ///< Short option with attached value (-pVALUE).
        bool has_attached = false;   ///< Token carries an attached value (long
                                     ///< --opt=V or short compact -pVALUE).
        std::string_view attached;   ///< Attached value text; for the short compact
                                     ///< form exactly one leading '=' is stripped
                                     ///< (mirrors consume_short).  Valid only when
                                     ///< has_attached.
    };

    /// @brief A resolved option plus how many parent hops away it is declared.
    struct ChainOptionMatch
    {
        const OptionDef *option = nullptr;  ///< Matching option (null if none).
        std::size_t depth = 0;              ///< 0 == the starting command itself.
    };

    /// @brief Find @p name on @p cmd or its nearest ancestor declaring it.
    /// @param cmd   Command to start from.
    /// @param name  Long option name without the `--` prefix.
    /// @return Matching option plus its declaring depth (null option if none).
    inline ChainOptionMatch find_option_by_long_in_chain_with_depth(
        const BaseCommand &cmd, std::string_view name) noexcept
    {
        std::size_t depth = 0;
        for (const auto *cur = &cmd; cur != nullptr; cur = cur->parent(), ++depth)
            if (const auto *opt = cur->find_option_by_long(name))
                return {opt, depth};
        return {};
    }

    /// @brief Find short option @p c on @p cmd or its nearest ancestor.
    /// @param cmd  Command to start from.
    /// @param c    Short option character.
    /// @return Matching option plus its declaring depth (null option if none).
    inline ChainOptionMatch find_option_by_short_in_chain_with_depth(
        const BaseCommand &cmd, char c) noexcept
    {
        std::size_t depth = 0;
        for (const auto *cur = &cmd; cur != nullptr; cur = cur->parent(), ++depth)
            if (const auto *opt = cur->find_option_by_short(c))
                return {opt, depth};
        return {};
    }

    /// @brief Find @p name on @p cmd or its nearest ancestor declaring it.
    /// @param cmd   Command to start from.
    /// @param name  Long option name without the `--` prefix.
    /// @return Matching option, or nullptr.
    inline const OptionDef *find_option_by_long_in_chain(
        const BaseCommand &cmd, std::string_view name) noexcept
    {
        return find_option_by_long_in_chain_with_depth(cmd, name).option;
    }

    /// @brief Find short option @p c on @p cmd or its nearest ancestor.
    /// @param cmd  Command to start from.
    /// @param c    Short option character.
    /// @return Matching option, or nullptr.
    inline const OptionDef *find_option_by_short_in_chain(
        const BaseCommand &cmd, char c) noexcept
    {
        return find_option_by_short_in_chain_with_depth(cmd, c).option;
    }

    /// @brief Find the option with @p hash on @p cmd or its nearest ancestor.
    /// @param cmd   Command to start from.
    /// @param hash  Compile-time key hash of the option.
    /// @return Matching option plus its declaring depth (null option if none).
    inline ChainOptionMatch find_option_by_hash_in_chain_with_depth(
        const BaseCommand &cmd, std::size_t hash) noexcept
    {
        std::size_t depth = 0;
        for (const auto *cur = &cmd; cur != nullptr; cur = cur->parent(), ++depth)
            if (const auto *opt = cur->find_option_by_hash(hash))
                return {opt, depth};
        return {};
    }

    /// @brief Find the option with @p hash on @p cmd or its nearest ancestor.
    /// @param cmd   Command to start from.
    /// @param hash  Compile-time key hash of the option.
    /// @return Matching option, or nullptr.
    inline const OptionDef *find_option_by_hash_in_chain(
        const BaseCommand &cmd, std::size_t hash) noexcept
    {
        return find_option_by_hash_in_chain_with_depth(cmd, hash).option;
    }

    /// @brief Interpret one dash-prefixed token against @p command's option chain.
    ///
    /// Mirrors OptionConsumer's token grammar for scanning (not parsing):
    /// `--opt`, `--opt=value`, `--no-opt`, grouped short flags, compact
    /// `-pVALUE`, and separate `-p VALUE`.  @c compact_value distinguishes a
    /// short option whose value is attached from a long `--opt=value`; the
    /// parser applies repeatable greedy consumption to the former and to
    /// separate values, but not to `--opt=value`.  @c has_attached is true for
    /// both attached forms, with @c attached exposing the value text (one
    /// leading '=' stripped for the short form, mirroring consume_short).
    ///
    /// @param command  Command in scope for option lookup (ancestors searched).
    /// @param token    Token starting with '-'.
    /// @return Resolved option plus how its value is supplied.
    inline OptionTokenScan scan_option_token(
        const BaseCommand &command, std::string_view token)
    {
        OptionTokenScan scan;

        if (token.size() >= 2 && token[0] == '-' && token[1] == '-')
        {
            auto lo = Tokenizer::parse_long_option(token);
            const auto *opt = find_option_by_long_in_chain(command, lo.name);
            if (!opt && lo.is_negation)
                opt = find_option_by_long_in_chain(command, lo.negated_name);
            scan.option = opt;
            if (opt && opt->has_value())
            {
                if (lo.has_equals)
                {
                    scan.has_attached = true;
                    scan.attached = lo.value;
                }
                else
                {
                    scan.needs_next_token = true;
                }
            }
            return scan;
        }

        // Short token: grouped flags and compact values, mirroring consume_short.
        for (std::size_t i = 1; i < token.size(); ++i)
        {
            const auto *opt = find_option_by_short_in_chain(command, token[i]);
            if (!opt)
                return scan;
            if (opt->has_value())
            {
                scan.option = opt;
                if (i + 1 == token.size())
                {
                    scan.needs_next_token = true;  // value arrives as next token.
                }
                else
                {
                    scan.compact_value = true;  // remainder is an attached value.
                    scan.has_attached = true;
                    scan.attached = token.substr(i + 1);
                    if (scan.attached.front() == '=')
                        scan.attached.remove_prefix(1);
                }
                return scan;
            }
        }
        return scan;
    }

}  // namespace pjh::cli::detail

#endif  // INCLUDE_PJH_CLI_COMMAND_ARG_SCAN_HPP
