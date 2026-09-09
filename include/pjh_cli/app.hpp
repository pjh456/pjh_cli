#ifndef INCLUDE_PJH_CLI_APP_HPP
#define INCLUDE_PJH_CLI_APP_HPP

#include <pjh_cli/command/branch_command.hpp>
#include <pjh_cli/core/type.hpp>
#include <pjh_cli/detail/env_snapshot.hpp>
#include <pjh_cli/parse/parse_context.hpp>
#include <pjh_cli/parse/parser.hpp>
#include <string>
#include <utility>

namespace pjh::cli
{
    /// @brief Application entry point — the root branch command.
    ///
    /// Represents the entire CLI application.  Owns the top-level command
    /// tree (via BranchCommand inheritance), holds version metadata, and
    /// provides parse() / parse_fuzzy() as the main entry points.
    ///
    /// Usage:
    /// @code
    ///   App app("git", "2.40.0", "The stupid content tracker");
    ///   app.option<fixed_string("verbose")>("--verbose", 'v', "Verbose").count();
    ///   auto &clone = app.add_leaf("clone", "Clone a repository");
    ///   clone.arg<std::string, 0>("url", "Repository URL").required();
    ///   auto r = app.parse(argc, argv);
    /// @endcode
    class App final : public BranchCommand
    {
    public:
        /// @brief Construct an application root.
        /// @param name        Binary / subcommand display name shown in usage.
        /// @param version     Version string (e.g. "1.0.0").
        /// @param description Short description shown in help text.
        App(std::string name, std::string version, std::string description);

        /// @brief The version string passed at construction.
        const std::string &version() const noexcept override { return m_version; }

        /// @brief Environment snapshot taken at construction.
        const detail::EnvSnapshot *env_snapshot() const noexcept override
        {
            return &m_env_snapshot;
        }

        /// @brief Override how batch --help / -h is rendered.
        ///
        /// The formatter receives the command whose help was requested; its
        /// non-empty return value becomes ParseContext::help_text().  Pass an
        /// empty function to restore the built-in HelpFormatter::format_help.
        /// The formatter must return a non-empty string: help_requested() is
        /// derived from help_text() being non-empty.
        /// @param formatter  Renderer, or {} for the built-in default.
        void set_help_formatter(HelpFormatterFn formatter)
        {
            m_help_formatter = std::move(formatter);
        }

        /// @brief The current batch help formatter (empty = built-in).
        const HelpFormatterFn &help_formatter() const noexcept
        {
            return m_help_formatter;
        }

        /// @brief Parse CLI arguments (exact subcommand matching).
        ///
        /// Delegates to Parser::parse_command() with max_fuzzy_distance = 0.
        /// On success the returned ParseContext can be queried with
        /// get<T, Key>() / has<Key>().
        ///
        /// @note This does not print help/version or exit.  When
        ///       help_requested() / version_requested() is set, dispatch on it
        ///       and print help_text() / version_text() before reading values,
        ///       because the meta-flag path skips ParseFinalizer.
        ///
        /// @note --help / -h text is rendered by help_formatter(), defaulting to
        ///       HelpFormatter::format_help.
        ///
        /// @param argc Argument count from main().
        /// @param argv Argument vector from main().
        /// @return Ok(ParseContext) on success, or Err(CliError) on parse failure.
        CliResult<ParseContext> parse(int argc, char **argv);

        /// @brief Parse CLI arguments with fuzzy subcommand matching.
        ///
        /// When an exact subcommand match fails, falls back to Levenshtein
        /// distance matching (max_distance = 3).  If exactly one close match
        /// is found, it is used transparently (no error).  If more than one
        /// close match is found, returns `AmbiguousCommandError` listing the
        /// candidates instead of guessing; an exact/alias match always wins.
        ///
        /// @note This does not print help/version or exit.  When
        ///       help_requested() / version_requested() is set, dispatch on it
        ///       and print help_text() / version_text() before reading values,
        ///       because the meta-flag path skips ParseFinalizer.
        ///
        /// @note --help / -h text is rendered by help_formatter(), defaulting to
        ///       HelpFormatter::format_help.
        ///
        /// @param argc Argument count from main().
        /// @param argv Argument vector from main().
        /// @return Ok(ParseContext) on success, or Err(CliError) on parse failure.
        CliResult<ParseContext> parse_fuzzy(int argc, char **argv);

    private:
        detail::EnvSnapshot m_env_snapshot;
        std::string m_version;
        HelpFormatterFn m_help_formatter;
    };

}  // namespace pjh::cli

#endif  // INCLUDE_PJH_CLI_APP_HPP
