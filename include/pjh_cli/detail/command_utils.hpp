#ifndef INCLUDE_PJH_CLI_DETAIL_COMMAND_UTILS_HPP
#define INCLUDE_PJH_CLI_DETAIL_COMMAND_UTILS_HPP

#include <format>
#include <string>
#include <string_view>

#include <pjh_cli/command/base_command.hpp>
#include <pjh_cli/option_def.hpp>
#include <pjh_cli/detail/string_utils.hpp>

namespace pjh::cli::detail
{

    /// @brief Check whether a command should be listed in UI output.
    ///
    /// Returns true only if the command is enabled and its visibility
    /// mask includes the requested @p mode.
    ///
    /// @param cmd   The command to check.
    /// @param mode  The active visibility mode (Repl, Cli, Both, Hidden).
    /// @return true if the command is visible and enabled.
    inline bool is_visible_and_enabled(const BaseCommand &cmd, Visibility mode) noexcept
    {
        if (!cmd.is_enabled())
            return false;
        if ((cmd.visibility() & mode) == Visibility::Hidden)
            return false;
        return true;
    }

    /// @brief Build a display label for an OptionDef (used in usage lines).
    ///
    /// Produces labels like:
    ///   - `"-v"`
    ///   - `"--verbose"`
    ///   - `"-v, --verbose"`
    ///   - `"-p, --port PORT"`
    ///
    /// The value placeholder is the option name uppercased.
    /// Intended for format_usage() rendering directly from the command tree
    /// (as opposed to option_label(OptionInfo) in HelpFormatter for the
    /// pre-collected help path).
    ///
    /// @param opt  Option definition from the command tree.
    /// @param sep  Separator between short and long names (default ", ").
    /// @return Formatted label string.
    inline std::string option_left_label(
        const OptionDef &opt, std::string_view sep = ", ")
    {
        std::string left;
        if (opt.short_name() != 0)
            left = std::format("-{}", opt.short_name());
        if (opt.short_name() != 0 && !opt.long_name().empty())
            left += sep;
        if (!opt.long_name().empty())
            left += "--" + opt.long_name();
        if (opt.has_value())
        {
            auto label = opt.long_name().empty() ? std::string(1, opt.short_name())
                                                 : opt.long_name();
            left += " " + StringUtils::to_upper_copy(label);
        }
        return left;
    }

}  // namespace pjh::cli::detail

#endif
