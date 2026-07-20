#ifndef INCLUDE_PJH_CLI_HINT_HPP
#define INCLUDE_PJH_CLI_HINT_HPP

#include <string>
#include <string_view>

#include "command/base_command.hpp"

namespace pjh::cli
{

    /// @brief Controls which options appear in the hint string.
    enum class HintOptionMode
    {
        All,       ///< Show all registered options (default).
        Required,  ///< Show only required options.
        None,      ///< Show no options (only positional args).
    };

    /// @brief Configuration for format_hint().
    struct HintConfig
    {
        HintOptionMode option_mode = HintOptionMode::All;
    };

    /// @brief Human-readable type name for an option (e.g. "INT", "STR").
    std::string option_type_name(const OptionDef &opt);

    /// @brief Format an interactive hint for the command reached by parsing @p input.
    ///
    /// The input string is tokenised and walked through the command tree:
    ///   - Tokens starting with `-` are treated as already-consumed options (skipped).
    ///   - Remaining tokens: if they match a subcommand name → descend; otherwise
    ///     they are treated as positional arg values (consumed from the front).
    ///
    /// The output looks like:
    ///   `INT:port [BOOL:verbose] <source> <dest>`
    ///
    /// @param root   Root of the command tree.
    /// @param input  Partial command line string.
    /// @param config Formatting options (which options to show).
    /// @return A space-separated hint string, or empty if nothing is available.
    std::string format_hint(
        const BaseCommand &root, std::string_view input, HintConfig config = {});

}  // namespace pjh::cli

#endif
