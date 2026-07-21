#ifndef INCLUDE_PJH_CLI_HINT_HPP
#define INCLUDE_PJH_CLI_HINT_HPP

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

#include "command/base_command.hpp"
#include "info.hpp"

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

    /// @brief Walk a partial input string to determine the current command position.
    HintContext build_hint_context(const BaseCommand &root, std::string_view input);

    /// @brief Render HintContext as a hint string showing all available options and args.
    std::string format_hint(const HintContext &ctx);

    /// @brief Format an interactive hint for the command reached by parsing @p input.
    ///
    /// Internally calls build_hint_context() and renders the result.
    /// For direct access to the structured data, use build_hint_context() directly.
    ///
    /// @param root   Root of the command tree.
    /// @param input  Partial command line string.
    /// @param config Formatting options (which options to show).
    /// @return A space-separated hint string, or empty if nothing is available.
    std::string format_hint(
        const BaseCommand &root, std::string_view input, HintConfig config = {});

}  // namespace pjh::cli

#endif
