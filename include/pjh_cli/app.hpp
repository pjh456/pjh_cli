#ifndef INCLUDE_PJH_CLI_APP_HPP
#define INCLUDE_PJH_CLI_APP_HPP

#include "command.hpp"

namespace pjh::cli
{
    /// @brief Application entry point — the root command.
    ///
    /// Inherits Command and adds version metadata, parse(), and run_interactive().
    class App final : public Command
    {
    public:
        /// @brief Construct an application root.
        /// @param name        Binary / application name.
        /// @param version     Version string.
        /// @param description Short description shown in help.
        App(std::string name, std::string version, std::string description);

        /// @brief The version string.
        const std::string &version() const noexcept { return m_version; }

        /// @brief Parse command-line arguments and produce a ParseContext.
        /// @param argc Argument count (from main).
        /// @param argv Argument vector (from main).
        /// @return ParseContext on success, or a CliError with details.
        CliResult<ParseContext> parse(int argc, char **argv);

        /// @brief Parse with fuzzy subcommand matching.
        ///
        /// When an exact subcommand match fails, falls back to Levenshtein
        /// distance matching (max_distance = 3). If exactly one close match
        /// is found, it is used transparently.
        /// @return ParseContext on success, or a CliError with details.
        CliResult<ParseContext> parse_fuzzy(int argc, char **argv);

    private:
        std::string m_version;
    };

}  // namespace pjh::cli

#endif  // INCLUDE_PJH_CLI_APP_HPP
