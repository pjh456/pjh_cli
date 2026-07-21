#ifndef INCLUDE_PJH_CLI_PARSER_HPP
#define INCLUDE_PJH_CLI_PARSER_HPP

#include <span>
#include <string_view>
#include <vector>

#include "command/branch_command.hpp"
#include "converter.hpp"
#include "parse_context.hpp"
#include "type.hpp"

namespace pjh::cli
{

    /// @brief Utility for parsing CLI arguments against a command tree.
    ///
    /// Walks the argument list once, navigating the command tree, consuming
    /// options (short and long, with =value next-token), handling --help/-h
    /// inline, descending into subcommands, matching positional arguments,
    /// and applying defaults / env-vars / required-checks in a final pass.
    ///
    /// All state lives on the stack — the class itself has no instance data.
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
        ///   2. `--help` / `-h`    → return help-only ParseContext immediately
        ///   3. `--opt` / `-x`     → consume option via consume_long/consume_short
        ///   4. word token         → try subcommand descent, then positional arg,
        ///                           then ExtraArgsPolicy dispatch
        ///   5. end of args        → finish_parse (defaults, env, required checks)
        ///
        /// @param root               Root of the command tree (typically an App
        ///                           instance).
        /// @param args               Tokenised CLI arguments (argv[1..]).  The caller
        ///                           is responsible for producing the span — see the
        ///                           argc/argv overload for the common case.
        /// @param max_fuzzy_distance  Max Levenshtein distance for subcommand fuzzy
        ///                           matching.  0 = exact only (default).
        /// @return Ok(ParseContext) on success, or Err(CliError) on parse failure.
        static CliResult<ParseContext> parse_command(
            BaseCommand &root,
            std::span<const std::string_view> args,
            int max_fuzzy_distance = 0);

        /// @brief Convenience overload: converts argv[1..argc-1] to a span of
        ///        string_views, then delegates to the span overload.
        ///
        /// @param root               Root of the command tree.
        /// @param argc               Argument count from main().
        /// @param argv               Argument vector from main().
        /// @param max_fuzzy_distance  0 = exact only (default).
        /// @return Ok(ParseContext) or Err(CliError).
        static CliResult<ParseContext> parse_command(
            BaseCommand &root, int argc, char **argv, int max_fuzzy_distance = 0)
        {
            std::vector<std::string_view> args;
            args.reserve(static_cast<size_t>(argc) - 1);
            for (int a = 1; a < argc; a++) args.emplace_back(argv[a]);
            return parse_command(root, args, max_fuzzy_distance);
        }

    private:
        /// @brief Internal result of try_descend_subcommand().
        struct SubcommandResult
        {
            bool matched = false;        ///< true if a subcommand was matched
            BaseCommand *cmd = nullptr;  ///< pointer to matched subcommand (owns nothing)
            ParseContext ctx;  ///< child context, parent-linked to the caller's ctx
        };

        /// @brief Dispatch a raw string value for a positional argument.
        ///
        /// Uses Converter<T>::from_string based on @p tag to parse the value,
        /// then stores it in @p ctx under @p hash.
        ///
        /// @param ctx   Parse context to write into.
        /// @param hash  Key hash from the ArgDef.
        /// @param tag   ValueTag indicating the target C++ type.
        /// @param s     Raw input string.
        /// @return Ok on success, or Err with a type-conversion error.
        static CliResult<void> apply_arg_value(
            ParseContext &ctx, size_t hash, ValueTag tag, std::string_view s);

        /// @brief Consume a single long-option token (--opt or --opt=val).
        ///
        /// Looks up the option on @p cmd.  If it expects a value, the value is
        /// taken from after `=` (if present) or from the next token in @p args.
        /// If the option is a flag (bool), sets it to true.  Counting options
        /// are incremented.  The special negation pattern (--no-xxx) is handled
        /// inline.
        ///
        /// @param cmd   Current command whose options are consulted.
        /// @param ctx   Parse context to write into.
        /// @param arg   The raw token (e.g. "--port=8080").
        /// @param i     Current index into @p args; advanced when a separate
        ///              value token is consumed.  The caller still increments
        ///              @p i to skip past the option after the call.
        /// @param args  Full argument list.
        /// @return Ok on success, or an appropriate CliError.
        static CliResult<void> consume_long(
            const BaseCommand &cmd,
            ParseContext &ctx,
            std::string_view arg,
            size_t &i,
            std::span<const std::string_view> args);

        /// @brief Consume a short-option token (-x, -abc, or -p value).
        ///
        /// Iterates over each character in the token:
        ///   - Bool flags are set to true directly.
        ///   - Counting options are incremented.
        ///   - Valued options require a standalone token (-p value);
        ///     grouped forms like -pvalue are rejected.
        /// When a valued option is found, @p i is advanced to consume the
        /// next token as its value.
        ///
        /// @param cmd   Current command whose options are consulted.
        /// @param ctx   Parse context to write into.
        /// @param arg   The raw token (e.g. "-abc" or "-p").
        /// @param i     Current index into @p args; advanced when a separate
        ///              value token is consumed.
        /// @param args  Full argument list.
        /// @return Ok on success, or an appropriate CliError.
        static CliResult<void> consume_short(
            const BaseCommand &cmd,
            ParseContext &ctx,
            std::string_view arg,
            size_t &i,
            std::span<const std::string_view> args);

        /// @brief Set a flag or increment a counter on @p ctx.
        ///
        /// Shared by consume_long() and consume_short().  If the option
        /// is a CountingOption, the stored int is incremented by 1;
        /// otherwise the option is treated as a boolean flag and set to true.
        ///
        /// @param opt  The matched option definition (must be non-null).
        /// @param ctx  Parse context to write into.
        static void apply_flag(const OptionDef *opt, ParseContext &ctx);

        /// @brief Convert a raw string to type T and store it in @p ctx.
        ///
        /// For bool/int/double the conversion uses Converter<T>::from_string;
        /// for string/path the value is constructed directly.  On conversion
        /// failure the CliError is propagated.
        /// @tparam T Target type (must satisfy BuiltinType).
        /// @param ctx  Parse context to write into.
        /// @param hash Key hash from the ArgDef.
        /// @param s    Raw input string.
        /// @return Ok or Err with a type-conversion error.
        template <detail::BuiltinType T>
        static CliResult<void> convert_and_set(
            ParseContext &ctx, size_t hash, std::string_view s)
        {
            if constexpr (std::same_as<T, std::string>)
            {
                ctx.set_value<std::string>(hash, std::string(s));
            }
            else if constexpr (std::same_as<T, std::filesystem::path>)
            {
                ctx.set_value<std::filesystem::path>(hash, std::filesystem::path(s));
            }
            else
            {
                auto r = Converter<T>::from_string(s);
                if (r.is_err())
                    return CliResult<void>::Err(std::move(r).unwrap_err());
                ctx.set_value<T>(hash, r.unwrap());
            }
            return CliResult<void>::Ok();
        }

        /// @brief Finalise parsing: apply defaults, env-vars, and required checks.
        ///
        /// Walks the full command chain from root down to the deepest matched
        /// command.  For each command:
        ///   1. Apply compile-time default values for options not yet set.
        ///   2. If an option has an env-var and no CLI value, read from environment.
        ///   3. If an option is required and still not set, return an error.
        ///   4. If a positional arg is required and not set, return an error.
        ///
        /// @param cmd  The deepest matched command.
        /// @param ctx  Parse context (parent chain already linked).
        /// @return Ok with the finalised context, or Err on missing required.
        static CliResult<ParseContext> finish_parse(BaseCommand *cmd, ParseContext ctx);

        /// @brief Exact + fuzzy subcommand lookup.
        ///
        /// Tries exact name match first.  If the exact match is disabled,
        /// sets @p out_disabled = true and returns nullptr.  If no exact
        /// match and @p max_fuzzy_distance > 0, tries fuzzy (Levenshtein)
        /// matching against all visible subcommands.  Returns the single
        /// closest fuzzy match if exactly one candidate is within the
        /// distance threshold.
        ///
        /// @param cmd               The parent BranchCommand.
        /// @param name              Subcommand name (user input).
        /// @param max_fuzzy_distance  Max edit distance (0 = exact only).
        /// @param out_disabled      Set to true if an exact match was found
        ///                          but is disabled (caller should return
        ///                          command_disabled).
        /// @return Pointer to the matched command, or nullptr.
        static BaseCommand *find_subcommand_match(
            BranchCommand &cmd,
            std::string_view name,
            int max_fuzzy_distance,
            bool &out_disabled);

        /// @brief If the current token is --help / -h, return a help-only
        ///        ParseContext immediately.
        ///
        /// The returned context has help_requested() == true.  The caller
        /// should return it as-is — the interactive console and App.parse()
        /// detect this flag and print help text instead of executing
        /// the command action.
        ///
        /// @param cmd         Current command.
        /// @param ctx         Parse context (moved in).
        /// @param a           Current argument token.
        /// @param double_dash  Whether we've already seen a `--` separator.
        /// @return Some(ParseContext) if this was a help request,
        ///         None() otherwise.
        static pjh::result::Option<ParseContext> try_handle_help(
            BaseCommand *cmd, ParseContext &&ctx, std::string_view a, bool double_dash);

        /// @brief If the token matches a (possibly fuzzy) subcommand, descend.
        ///
        /// On success the returned SubcommandResult contains the matched
        /// command and a fresh child ParseContext whose parent points to
        /// the old context (via shared_ptr).  The caller must replace its
        /// own cmd / ctx / arg_pos with these values.
        ///
        /// @param cmd               Current command (may be updated).
        /// @param ctx               Parse context (may be moved from on match).
        /// @param a                 Current argument token.
        /// @param max_fuzzy_distance Max edit distance for fuzzy matching.
        /// @param double_dash       Whether we've already seen `--`.
        /// @return Ok with .matched = true on success, Ok with .matched = false
        ///         when no match (caller should try positional arg), or
        ///         Err(command_disabled) if the exact match is disabled.
        static CliResult<SubcommandResult> try_descend_subcommand(
            BaseCommand *cmd,
            ParseContext &ctx,
            std::string_view a,
            int max_fuzzy_distance,
            bool double_dash);

        /// @brief Handle an unrecognised token per ExtraArgsPolicy.
        ///
        /// If the policy is Error, returns a parse_error.  If Store,
        /// appends the token to the extra-args list.  If Ignore, no-op.
        ///
        /// @param cmd  Current command whose policy is read.
        /// @param ctx  Parse context (extra args may be appended).
        /// @param a    Unrecognised token.
        /// @param pos  Argument position (for error message).
        /// @return Ok or Err if policy is Error.
        static CliResult<void> handle_extra_arg(
            const BaseCommand *cmd, ParseContext &ctx, std::string_view a, size_t pos);
    };

}  // namespace pjh::cli

#endif
