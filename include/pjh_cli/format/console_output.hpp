#ifndef INCLUDE_PJH_CLI_CONSOLE_OUTPUT_HPP
#define INCLUDE_PJH_CLI_CONSOLE_OUTPUT_HPP

#include <pjh_cli/format/info.hpp>
#include <string>
#include <string_view>
#include <vector>

namespace pjh::cli
{
    /// @brief Static utility for rendering InteractiveConsole output strings.
    ///
    /// Produces the standard REPL display text for query results, help
    /// navigation feedback, and error messages.  All methods return a
    /// plain std::string with no I/O side effects, making them usable
    /// from any embedding context (GUI, WebSocket, server).
    ///
    /// These formatters define the default REPL output style. Embedders
    /// who want different formatting can use the structured query/help
    /// result types and bypass these formatters entirely.
    class ConsoleOutput
    {
    public:
        ConsoleOutput() = delete;

        /// @brief Render fuzzy suggestions as a space-separated name list.
        /// @param info  SuggestionInfo from collect_fuzzy_suggestions().
        /// @return E.g. `" start stop"` (note the leading space).
        static std::string format_suggestions(const SuggestionInfo &info);

        /// @brief Message when a branch has no subcommands.
        /// @return `"No subcommands available."`
        static std::string format_no_subcommands();

        /// @brief Render the full subcommand list.
        /// @param names  Sorted subcommand names.
        /// @return E.g. `"Subcommands: foo bar"`.
        static std::string format_subcommand_list(
            const std::vector<std::string> &names);

        /// @brief Render substring-matched subcommands.
        /// @param names  Matching subcommand names.
        /// @return E.g. `"Matching subcommands: server"`.
        static std::string format_matched_subcommands(
            const std::vector<std::string> &names);

        /// @brief Render "no match" fallback with a usage hint.
        /// @param usage  Pre-formatted usage line (e.g. from HelpFormatter).
        /// @return E.g. `"No matches. Try: Usage: app [options]"`.
        static std::string format_no_match(const std::string &usage);

        /// @brief Message when a user tries to descend into a non-branch command.
        /// @param name  Command name.
        /// @return E.g. `"'serve' has no subcommands."`
        static std::string format_has_no_subcommands(const std::string &name);

        /// @brief Render unknown subcommand error with optional suggestions.
        /// @param name         Unmatched token.
        /// @param suggestions  Pre-formatted suggestions string (may be empty).
        /// @return E.g. `"Unknown subcommand 'servr'. Did you mean: server"`
        ///         or `"Unknown subcommand 'zzz'."`.
        static std::string format_unknown_subcommand(
            const std::string &name, std::string_view suggestions);
    };

}  // namespace pjh::cli

#endif