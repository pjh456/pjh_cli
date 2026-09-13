#ifndef INCLUDE_PJH_CLI_MATCHER_HPP
#define INCLUDE_PJH_CLI_MATCHER_HPP

#include <cstddef>
#include <pjh_cli/command/matcher.hpp>
#include <pjh_cli/format/info.hpp>
#include <string>
#include <string_view>
#include <vector>

/// @brief Completion helpers plus a re-export of the command-layer matching
///        primitives.
///
/// The fuzzy/list matching primitives (edit_distance, FuzzyMatch,
/// fuzzy_find_subcommands, list_subcommands) are declared in
/// command/matcher.hpp; this header re-exports them so existing includers keep
/// resolving those names.
namespace pjh::cli
{
    /// @brief Completion candidates for a partial token on a command.
    ///
    /// Matches subcommand names (when @p prefix does not start with '-') or
    /// option names (when @p prefix starts with '-').  Option completion
    /// handles long options (--ver→ --verbose) and short options (-v).
    /// Results are sorted and deduplicated.
    ///
    /// Options are resolved on @p cmd and its ancestors (nearest declaration
    /// wins); subcommands stay node-local.  Candidate buffers are reserved
    /// up front and the result is sorted in place.
    ///
    /// @param cmd    Current command whose options/subcommands are consulted.
    /// @param prefix Partial token to match against.
    /// @param mode   Visibility filter (default Both).
    /// @return Sorted, deduplicated candidate list (may be empty).
    std::vector<CompletionCandidate> complete_candidates(
        const BaseCommand &cmd,
        std::string_view prefix,
        Visibility mode = Visibility::Both);

    std::vector<std::string> complete(
        const BaseCommand &cmd,
        std::string_view prefix,
        Visibility mode = Visibility::Both);

    /// @brief Completion candidates for an option's value.
    ///
    /// Invokes OptionDef::completer_fn() and returns its entries whose text
    /// starts with @p prefix, sorted and deduplicated.  Empty when the option
    /// has no registered completer.
    ///
    /// Unlike complete_candidates(), which is name-only and never invokes a
    /// completer, this function is the entry point that makes `.completer(fn)`
    /// functional for option values.
    ///
    /// @param opt     Option whose completer supplies the candidates.
    /// @param prefix  Partial value typed so far (may be empty).
    /// @return Sorted, deduplicated candidates (may be empty).
    /// @throws std::bad_alloc if the candidate list cannot be allocated.
    /// @throws Any exception thrown by the registered completer is propagated.
    std::vector<CompletionCandidate> complete_value_candidates(
        const OptionDef &opt, std::string_view prefix);

    /// @brief Completion candidates for the token under @p cursor in @p line.
    ///
    /// Resolves the command reached by the tokens before the cursor and the
    /// option (if any) whose value is being typed, then returns value
    /// candidates (preceding option has a completer) or name candidates.
    /// Supports `--opt value`, `--opt=value`, `-o value`, and `-ovalue`.
    /// Option names and values both walk the ancestor chain (matching the
    /// parser); subcommand completion is node-local.
    ///
    /// @param root    Root of the command tree.
    /// @param line    Full input line.
    /// @param cursor  Byte offset of the cursor (0..line.size()).
    /// @param mode    Visibility filter for name candidates (default Both).
    /// @return Sorted, deduplicated candidates (may be empty).
    /// @throws std::bad_alloc if the candidate list cannot be allocated.
    /// @throws Any exception thrown by a registered completer is propagated.
    ///
    /// @note The token under @p cursor is bounded by scanning back over ASCII
    ///       space and tab only (detail::Tokenizer::is_separator); the boundary
    ///       is NOT quote- or escape-aware.  A leading quote or a backslash in
    ///       the token is kept in the matched prefix (e.g. `--opt "gr` matches
    ///       `"gr`, not `gr`), and a separator escaped with a backslash or
    ///       enclosed in an open quote is treated as a token boundary.
    ///       Completion inside a quoted or escaped value is best-effort and can
    ///       differ from the tokenizer used to execute the line.
    std::vector<CompletionCandidate> complete_line(
        const BaseCommand &root,
        std::string_view line,
        std::size_t cursor,
        Visibility mode = Visibility::Both);

    /// @brief Completion candidates for the token under @p cursor, with the
    ///        matched prefix length.
    ///
    /// Resolves the same context as complete_line(), but also reports the byte
    /// length of the partial name/value that the candidates were matched
    /// against.  For `--opt=value` and `-pVALUE` that prefix is only a suffix of
    /// the token under the cursor, so an editor completing a unique candidate
    /// appends `candidate.display.substr(result.prefix_len)` instead of assuming
    /// the whole trailing token is the prefix.
    ///
    /// @param root    Root of the command tree.
    /// @param line    Full input line.
    /// @param cursor  Byte offset of the cursor (0..line.size()).
    /// @param mode    Visibility filter for name candidates (default Both).
    /// @return Candidates plus the matched prefix byte length.
    /// @throws std::bad_alloc if the candidate list cannot be allocated.
    /// @throws Any exception thrown by a registered completer is propagated.
    ///
    /// @note The token under @p cursor is bounded by scanning back over ASCII
    ///       space and tab only (detail::Tokenizer::is_separator); the boundary
    ///       is NOT quote- or escape-aware.  A leading quote or a backslash in
    ///       the token is kept in the matched prefix (e.g. `--opt "gr` matches
    ///       `"gr`, not `gr`), and a separator escaped with a backslash or
    ///       enclosed in an open quote is treated as a token boundary.
    ///       Completion inside a quoted or escaped value is best-effort and can
    ///       differ from the tokenizer used to execute the line.
    CompletionResult complete_line_result(
        const BaseCommand &root,
        std::string_view line,
        std::size_t cursor,
        Visibility mode = Visibility::Both);

}  // namespace pjh::cli

#endif
