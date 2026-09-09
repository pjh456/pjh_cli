#ifndef INCLUDE_PJH_CLI_PARSE_SUBCOMMAND_RESOLVER_HPP
#define INCLUDE_PJH_CLI_PARSE_SUBCOMMAND_RESOLVER_HPP

#include <pjh_cli/command/base_command.hpp>
#include <pjh_cli/command/branch_command.hpp>
#include <pjh_cli/core/type.hpp>
#include <pjh_cli/parse/parse_context.hpp>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace pjh::cli
{
    /// @brief Resolves token strings to subcommands in the command tree.
    ///
    /// Supports exact name matching first, then optional Levenshtein fuzzy
    /// matching as a fallback.  Disabled commands are detected and reported
    /// distinctly from "not found".
    ///
    /// try_descend_subcommand() is the high-level entry: it checks for
    /// double-dash barriers, verifies the command is a branch, delegates to
    /// find_subcommand_match(), and creates a child ParseContext linked
    /// to the parent on success.
    class SubcommandResolver
    {
    public:
        /// @brief Result of a subcommand descent attempt.
        struct SubcommandResult
        {
            bool matched = false;       ///< true if a subcommand was matched.
            BaseCommand *cmd = nullptr; ///< Pointer to the matched subcommand.
            ParseContext ctx;           ///< Child context, parent-linked to
                                        ///< the caller's context.
        };

        SubcommandResolver() = delete;

        /// @brief Find a subcommand by name, trying exact then fuzzy match.
        ///
        /// Exact name match is attempted first (including aliases).  If the
        /// exact match is disabled, @p out_disabled is set to true and
        /// nullptr is returned.  When no exact match is found and
        /// @p max_fuzzy_distance > 0, fuzzy_find_subcommands() is used with
        /// a Levenshtein distance threshold.  Only when exactly one candidate
        /// falls within the threshold is it returned; when more than one
        /// candidate does, @p out_ambiguous is appended with the candidate
        /// command names and nullptr is returned.
        ///
        /// @param cmd                Parent BranchCommand to search.
        /// @param name               User-supplied subcommand name.
        /// @param max_fuzzy_distance  Max edit distance (0 = exact only).
        /// @param out_disabled       Set to true if an exact match was found
        ///                           but is disabled.
        /// @param out_ambiguous      Appended with the candidate command names
        ///                           (closest first) when more than one
        ///                           visible+enabled child is within
        ///                           @p max_fuzzy_distance; left untouched
        ///                           otherwise.
        /// @return Pointer to the matched BaseCommand, or nullptr.  A
        ///         non-empty @p out_ambiguous or true @p out_disabled also
        ///         yields nullptr.
        static BaseCommand *find_subcommand_match(
            BranchCommand &cmd,
            std::string_view name,
            int max_fuzzy_distance,
            bool &out_disabled,
            std::vector<std::string> &out_ambiguous);

        /// @brief Build an unknown-command error with fuzzy suggestions.
        ///
        /// Scans @p cmd's visible + enabled children for names and aliases
        /// within a fixed edit distance of @p input, closest first, and wraps
        /// them in a structured UnknownCommandError.  Read-only: does not
        /// affect fuzzy match acceptance or command descent.
        ///
        /// @param cmd    Branch whose children are searched for suggestions.
        /// @param input  Unmatched user token.
        /// @return CliError carrying the input and the closest suggestions.
        static CliError unknown_subcommand(BranchCommand &cmd, std::string_view input);

        /// @brief If the token matches a (possibly fuzzy) subcommand, descend.
        ///
        /// Guards against descent when @p double_dash is true, the token is
        /// dash-prefixed, or the current command is not a branch; a
        /// dash-prefixed token can never name an invocable subcommand, so such
        /// tokens return unmatched.  On success the returned SubcommandResult
        /// contains the matched command and a fresh child ParseContext whose
        /// parent points to the old context (via shared_ptr).  The caller
        /// must replace its own cmd / ctx with these values.
        ///
        /// @param cmd                Current command (may be updated on match).
        /// @param ctx                Parse context (moved from on match).
        /// @param a                  Current argument token.
        /// @param max_fuzzy_distance  Max edit distance for fuzzy matching.
        /// @param double_dash        Whether we have already seen '--'.
        /// @return Ok with .matched = true on match, Ok with .matched = false
        ///         when no match found, Err(command_disabled) if the exact
        ///         match is disabled, or Err(ambiguous_command) when more
        ///         than one fuzzy candidate is within threshold (the
        ///         candidate list is carried in
        ///         AmbiguousCommandError::candidates, closest first).
        static CliResult<SubcommandResult> try_descend_subcommand(
            BaseCommand *cmd,
            ParseContext &ctx,
            std::string_view a,
            int max_fuzzy_distance,
            bool double_dash);
    };
}  // namespace pjh::cli

#endif
