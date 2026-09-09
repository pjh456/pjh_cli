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
    };

    /// @brief Find @p name on @p cmd or its nearest ancestor declaring it.
    /// @param cmd   Command to start from.
    /// @param name  Long option name without the `--` prefix.
    /// @return Matching option, or nullptr.
    inline const OptionDef *find_option_by_long_in_chain(
        const BaseCommand &cmd, std::string_view name) noexcept
    {
        for (const auto *cur = &cmd; cur != nullptr; cur = cur->parent())
            if (const auto *opt = cur->find_option_by_long(name))
                return opt;
        return nullptr;
    }

    /// @brief Find short option @p c on @p cmd or its nearest ancestor.
    /// @param cmd  Command to start from.
    /// @param c    Short option character.
    /// @return Matching option, or nullptr.
    inline const OptionDef *find_option_by_short_in_chain(
        const BaseCommand &cmd, char c) noexcept
    {
        for (const auto *cur = &cmd; cur != nullptr; cur = cur->parent())
            if (const auto *opt = cur->find_option_by_short(c))
                return opt;
        return nullptr;
    }

    /// @brief Interpret one dash-prefixed token against @p command's option chain.
    ///
    /// Mirrors OptionConsumer's token grammar for scanning (not parsing):
    /// `--opt`, `--opt=value`, `--no-opt`, grouped short flags, compact
    /// `-pVALUE`, and separate `-p VALUE`.  @c compact_value distinguishes a
    /// short option whose value is attached from a long `--opt=value`; the
    /// parser applies repeatable greedy consumption to the former and to
    /// separate values, but not to `--opt=value`.
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
            if (opt && !lo.has_equals && opt->has_value())
                scan.needs_next_token = true;
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
                    scan.needs_next_token = true;  // value arrives as next token.
                else
                    scan.compact_value = true;  // remainder is an attached value.
                return scan;
            }
        }
        return scan;
    }

}  // namespace pjh::cli::detail

#endif  // INCLUDE_PJH_CLI_COMMAND_ARG_SCAN_HPP
