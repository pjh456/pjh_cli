#ifndef INCLUDE_PJH_CLI_APP_HPP
#define INCLUDE_PJH_CLI_APP_HPP

#include <iosfwd>
#include <pjh_cli/command/branch_command.hpp>
#include <pjh_cli/command/command_builder.hpp>
#include <pjh_cli/core/type.hpp>
#include <pjh_cli/detail/env_snapshot.hpp>
#include <pjh_cli/parse/parse_context.hpp>
#include <pjh_cli/parse/parser.hpp>
#include <string>
#include <string_view>
#include <utility>

namespace pjh::cli
{
    /// @brief Exit code returned by App::run() / App::run_fuzzy() on success,
    ///        or after printing help/version.
    inline constexpr int kExitSuccess = 0;

    /// @brief Exit code returned when the matched action failed with an
    ///        ErrorKind::Runtime error.
    inline constexpr int kExitRuntimeError = 1;

    /// @brief Exit code returned when parsing/validation failed with an
    ///        ErrorKind::Parse error (unknown option, missing value, …).
    inline constexpr int kExitParseError = 2;

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
    ///   // or one-shot: parse + dispatch help/version + execute + exit code
    ///   return app.run(argc, argv);
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

        /// @brief Look up an environment value from the snapshot taken at
        ///        construction.
        ///
        /// @param name Environment variable name (OptionDef::env_var()).
        /// @return Pointer to the stored value, or nullptr if absent.
        /// @throws std::bad_alloc if the lookup key cannot be allocated.
        const std::string *env_value(std::string_view name) const override
        {
            return m_env_snapshot.get(name);
        }

        /// @brief Override how batch --help / -h is rendered.
        ///
        /// The formatter receives the command whose help was requested; its
        /// non-empty return value becomes ParseContext::help_text().  Pass an
        /// empty function to restore the built-in HelpFormatter::format_help.
        /// The formatter must return a non-empty string: help_requested() is
        /// derived from help_text() being non-empty.
        ///
        /// The formatter governs parse() / parse_fuzzy() and the REPL's
        /// `cmd --help` path; InteractiveConsole reads it through the
        /// BranchCommand::help_formatter() override.
        /// @param formatter  Renderer, or {} for the built-in default.
        void set_help_formatter(HelpFormatterFn formatter)
        {
            m_help_formatter = std::move(formatter);
        }

        /// @brief The current batch help formatter (empty = built-in).
        ///
        /// Also used by InteractiveConsole for the REPL `cmd --help` path via the
        /// BranchCommand::help_formatter() override.
        const HelpFormatterFn &help_formatter() const noexcept override
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
        ///       HelpFormatter::format_help.  The same formatter drives the REPL
        ///       `cmd --help` path.
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
        ///       HelpFormatter::format_help.  The same formatter drives the REPL
        ///       `cmd --help` path.
        ///
        /// @param argc Argument count from main().
        /// @param argv Argument vector from main().
        /// @return Ok(ParseContext) on success, or Err(CliError) on parse failure.
        CliResult<ParseContext> parse_fuzzy(int argc, char **argv);

        /// @brief One-shot batch entry: parse, dispatch help/version, execute
        ///        the matched action, print errors, and return a process exit
        ///        code.
        ///
        /// Equivalent to the manual block every caller used to write:
        /// @code
        ///   auto r = app.parse(argc, argv);
        ///   if (r.is_err()) { std::cerr << r.unwrap_err().what() << "\n"; return 1; }
        ///   auto &ctx = r.unwrap();
        ///   if (ctx.help_requested())    { std::cout << ctx.help_text();    return 0; }
        ///   if (ctx.version_requested()) { std::cout << ctx.version_text(); return 0; }
        ///   auto e = ctx.matched_command()->execute(ctx);
        ///   if (e.is_err()) { std::cerr << e.unwrap_err().what() << "\n"; return 1; }
        ///   return 0;
        /// @endcode
        ///
        /// help_text() / version_text() are written to @p out exactly as
        /// produced by the parser (the injectable help formatter set via
        /// set_help_formatter() is honoured through parse()).  Errors are
        /// written to @p err as `what() << "\n"`; runtime action errors have
        /// no "Parse Error: " prefix (ErrorKind).  Does not call std::exit
        /// and does not catch exceptions.
        ///
        /// @param argc Argument count from main().
        /// @param argv Argument vector from main().
        /// @param out  Stream for help/version output.
        /// @param err  Stream for error messages.
        /// @return kExitSuccess (0) on success or help/version;
        ///         kExitRuntimeError (1) when the action returned an
        ///         ErrorKind::Runtime error; kExitParseError (2) on a
        ///         parse/validation failure.
        [[nodiscard]] int run(int argc, char **argv);

        /// @copydetails run(int, char **)
        [[nodiscard]] int run(
            int argc, char **argv, std::ostream &out, std::ostream &err);

        /// @brief run() with fuzzy subcommand matching (distance 3).
        /// @copydetails run(int, char **)
        [[nodiscard]] int run_fuzzy(int argc, char **argv);

        /// @copydetails run_fuzzy(int, char **)
        [[nodiscard]] int run_fuzzy(
            int argc, char **argv, std::ostream &out, std::ostream &err);

    private:
        detail::EnvSnapshot m_env_snapshot;
        std::string m_version;
        HelpFormatterFn m_help_formatter;
    };

}  // namespace pjh::cli

#endif  // INCLUDE_PJH_CLI_APP_HPP
