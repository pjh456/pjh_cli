#ifndef INCLUDE_PJH_CLI_PARSER_HPP
#define INCLUDE_PJH_CLI_PARSER_HPP

#include <functional>
#include <pjh_cli/command/branch_command.hpp>
#include <pjh_cli/core/type.hpp>
#include <pjh_cli/detail/help_formatter.hpp>
#include <pjh_cli/parse/parse_context.hpp>
#include <span>
#include <string_view>
#include <vector>

namespace pjh::cli
{
    /// @brief Signature of an injectable batch help formatter.
    ///
    /// Receives the command whose help was requested and returns the text stored
    /// in ParseContext::help_text().  An empty std::function selects the built-in
    /// HelpFormatter::format_help.  A custom formatter must return a non-empty
    /// string: help_requested() is derived from help_text() being non-empty.
    using HelpFormatterFn = detail::HelpFormatterFn;

    /// @brief Command-line argument parser for a command tree.
    ///
    /// Walks the argument list once, delegating to helper components:
    ///   - OptionConsumer  for --opt and -x tokens
    ///   - SubcommandResolver  for navigating into subcommands
    ///   - ValueWriter  for positional argument conversion
    ///   - ParseFinalizer  for post-parse validation
    ///
    /// The class has no instance data — all state lives on the stack during
    /// the parse_command() call.  Thread-safe.  `--help` / `-h` text is produced
    /// by an injectable formatter (see HelpFormatterFn), defaulting to
    /// HelpFormatter.
    ///
    /// Usage:
    /// @code
    ///   auto r = Parser::parse_command(app, argc, argv);
    ///   if (r.is_ok()) { auto &ctx = r.unwrap(); /* use ctx.get<T,Key>(...) */ }
    /// @endcode
    class Parser
    {
    public:
        Parser() = delete;

        /// @brief Parse a span of string_views against the command tree.
        ///
        /// The main parse entry point.  Walks @p args in a single pass:
        ///   1. `--`               → double-dash terminator
        ///   2. `--help` / `-h`    → return help-only context immediately
        ///   3. `--version`        → return version-only context immediately
        ///   4. `--opt` / `-x`     → delegate to OptionConsumer; a `-<digit>` /
        ///                           `-.` token is a value, not an option, and
        ///                           falls through to steps 5/6
        ///   5. word token         → try SubcommandResolver::try_descend,
        ///                           then positional arg via ValueWriter,
        ///                           then ExtraArgsPolicy dispatch
        ///   6. end of args        → ParseFinalizer::finalize
        ///
        /// @param root               Root of the command tree (App instance).
        /// @param args               Tokenised CLI arguments (argv[1..]).
        /// @param max_fuzzy_distance  Max Levenshtein distance for subcommand
        ///                           fuzzy matching.  0 = exact only (default).
        /// @param help_fmt           Help renderer for --help / -h; empty selects
        ///                           HelpFormatter::format_help (the default).
        /// @return Ok(ParseContext) on success, or Err(CliError) on failure.
        static CliResult<ParseContext> parse_command(
            BaseCommand &root,
            std::span<const std::string_view> args,
            int max_fuzzy_distance = 0,
            HelpFormatterFn help_fmt = detail::default_format_help);

        /// @brief Convenience: converts argv[1..argc-1] to a span and
        ///        delegates to the span overload.
        ///
        /// @param root               Root of the command tree.
        /// @param argc               Argument count from main().
        /// @param argv               Argument vector from main().
        /// @param max_fuzzy_distance  0 = exact only (default).
        /// @param help_fmt           Help renderer for --help / -h; empty selects
        ///                           HelpFormatter::format_help (the default).
        /// @return Ok(ParseContext) or Err(CliError).
        static CliResult<ParseContext> parse_command(
            BaseCommand &root,
            int argc,
            char **argv,
            int max_fuzzy_distance = 0,
            HelpFormatterFn help_fmt = detail::default_format_help);

    private:
        /// @brief If the current token is --help or -h, return a help-only
        ///        ParseContext immediately.
        ///
        /// The returned context has help_requested() == true.  The caller
        /// should check this flag and print help_text() instead of executing.
        ///
        /// @param cmd         Current command.
        /// @param ctx         Parse context (moved in).
        /// @param a           Current argument token.
        /// @param double_dash Whether we have already seen '--'.
        /// @param help_fmt    Help renderer; empty selects
        ///                    HelpFormatter::format_help.
        /// @return Some(ctx) if this was a help request, None() otherwise.
        static pjh::result::Option<ParseContext> try_handle_help(
            BaseCommand *cmd,
            ParseContext &&ctx,
            std::string_view a,
            bool double_dash,
            const HelpFormatterFn &help_fmt);

        /// @brief If the current token is --version, return a version-only
        ///        ParseContext immediately.
        ///
        /// The returned context has version_requested() == true.  The caller
        /// should print version_text() instead of executing the action.
        ///
        /// @param root        Root command (App) to read version from.
        /// @param ctx         Parse context (moved in).
        /// @param a           Current argument token.
        /// @param double_dash Whether we have already seen '--'.
        /// @return Some(ctx) if this was a version request, None() otherwise.
        static pjh::result::Option<ParseContext> try_handle_version(
            BaseCommand &root, ParseContext &&ctx, std::string_view a, bool double_dash);

        /// @brief Handle an unrecognised token per ExtraArgsPolicy.
        ///
        /// On a branch that has subcommands, the implicit `Ignore` default is
        /// overridden to an unknown-command error with fuzzy suggestions,
        /// unless the policy was set explicitly or a `--` barrier is active.
        /// Otherwise: Error returns a parse_error, Store appends to the parse
        /// root's extra_args() so tokens survive subcommand descent, Ignore is a
        /// no-op.
        ///
        /// @param cmd          Current command whose policy is read.
        /// @param ctx          Parse context (extra args may be appended).
        /// @param a            Unrecognised token.
        /// @param pos          Argument position (for error messages).
        /// @param double_dash  Whether we have already seen '--'.
        /// @return Ok or Err if policy is Error or an unknown command.
        static CliResult<void> handle_extra_arg(
            BaseCommand *cmd,
            ParseContext &ctx,
            std::string_view a,
            size_t pos,
            bool double_dash);
    };
}  // namespace pjh::cli

#endif
