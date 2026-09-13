#include <format>
#include <pjh_cli/format/console_output.hpp>
#include <string>
#include <string_view>
#include <vector>

namespace pjh::cli
{

    /// @brief Renders fuzzy suggestions as a space-prefixed name list.
    ///
    /// @param info  The SuggestionInfo containing match results.
    /// @return A string like `" start stop"` (empty string if no matches).
    std::string ConsoleOutput::format_suggestions(const SuggestionInfo &info)
    {
        std::string out;
        for (auto &m : info.matches) out += " " + m.name;
        return out;
    }

    /// @brief Returns the static "no subcommands" message.
    /// @return `"No subcommands available."`
    std::string ConsoleOutput::format_no_subcommands()
    {
        return "No subcommands available.";
    }

    /// @brief Renders a full subcommand listing.
    ///
    /// Delegates to format_no_subcommands() when @p names is empty.
    ///
    /// @param names  Subcommand names (may be empty).
    /// @return `"Subcommands: foo bar"` or `"No subcommands available."`.
    std::string ConsoleOutput::format_subcommand_list(
        const std::vector<std::string> &names)
    {
        if (names.empty())
            return format_no_subcommands();
        std::string out = "Subcommands:";
        for (auto &n : names) out += " " + n;
        return out;
    }

    /// @brief Renders a substring-matched subcommand listing.
    /// @param names  Matching subcommand names (non-empty in practice).
    /// @return `"Matching subcommands: server config"`.
    std::string ConsoleOutput::format_matched_subcommands(
        const std::vector<std::string> &names)
    {
        std::string out = "Matching subcommands:";
        for (auto &n : names) out += " " + n;
        return out;
    }

    /// @brief Renders the "no match" fallback message with a usage hint.
    /// @param usage  Pre-formatted usage string.
    /// @return `"No matches. Try: Usage: myapp [options]"`.
    std::string ConsoleOutput::format_no_match(const std::string &usage)
    {
        return std::format("No matches. Try: {}", usage);
    }

    /// @brief Renders the "has no subcommands" error for a non-branch command.
    /// @param name  The command name that was descended into.
    /// @return `"'serve' has no subcommands."`.
    std::string ConsoleOutput::format_has_no_subcommands(const std::string &name)
    {
        return std::format("'{}' has no subcommands.", name);
    }

    /// @brief Renders an unknown-subcommand error, optionally with "Did you mean:".
    ///
    /// When @p suggestions is non-empty, includes the fuzzy suggestion list.
    ///
    /// @param name         The unrecognised subcommand token.
    /// @param suggestions  Pre-formatted suggestions (empty string if none).
    /// @return `"Unknown subcommand 'servr'. Did you mean: server"` or
    ///         `"Unknown subcommand 'zzz'."`.
    std::string ConsoleOutput::format_unknown_subcommand(
        const std::string &name, std::string_view suggestions)
    {
        if (suggestions.empty())
            return std::format("Unknown subcommand '{}'.", name);
        return std::format("Unknown subcommand '{}'. Did you mean:{}", name, suggestions);
    }

}  // namespace pjh::cli