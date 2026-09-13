#ifndef INCLUDE_PJH_CLI_COMMAND_MATCHER_HPP
#define INCLUDE_PJH_CLI_COMMAND_MATCHER_HPP

#include <algorithm>
#include <cstddef>
#include <pjh_cli/command/base_command.hpp>
#include <pjh_cli/command/branch_command.hpp>
#include <string>
#include <string_view>
#include <vector>

namespace pjh::cli
{
    /// @brief Compute Levenshtein edit distance between two strings.
    ///
    /// The two DP rows live on the stack for candidates of at most 63
    /// characters; only the vector fallback for longer candidates allocates.
    ///
    /// @param a First string.
    /// @param b Second string.
    /// @return Number of single-character edits (insert/delete/substitute) needed.
    /// @throws std::bad_alloc on the vector fallback (candidates over 63 chars).
    int edit_distance(std::string_view a, std::string_view b);

    /// @brief A fuzzy match result returned by fuzzy_find_subcommands().
    struct FuzzyMatch
    {
        BaseCommand *command;  ///< The matched subcommand.
        int distance;          ///< Levenshtein distance (lower = closer).
    };

    /// @brief Find visible + enabled subcommands whose name fuzzily matches @p input.
    ///
    /// Enumerates all direct children of @p parent, applies the visibility
    /// filter and the enabled filter, then computes edit_distance() against
    /// each name.  Enabled results within @p max_distance are returned sorted
    /// by distance.  Names and aliases whose length differs from @p input by
    /// more than @p max_distance are rejected without running the distance
    /// (necessary condition; the accepted set is unchanged).
    ///
    /// Visible-but-disabled children never appear in the returned vector; when
    /// @p disabled_out is non-null they are collected there instead.  Hidden
    /// children (including hidden + disabled) are filtered before collection
    /// and are never reported in either place.
    ///
    /// Read-only: @p parent is not mutated.  The returned FuzzyMatch::command
    /// pointers are non-owning aliases into the live command tree (valid only
    /// while the tree lives); their pointee is mutable because the parse path
    /// uses a match to descend and execute, mirroring subcommands().
    ///
    /// @param parent       Parent branch to search (read-only).
    /// @param input        User input (potentially misspelled).
    /// @param max_distance  Max edit distance to accept (default 3).
    /// @param mode         Visibility filter (default Both).
    /// @param disabled_out  When non-null, appended with visible-but-disabled
    ///                      children that fall within @p max_distance (hidden
    ///                      children are never reported).  Null (default)
    ///                      keeps the enabled-only behavior and skips distance
    ///                      computation for disabled children.
    /// @return Sorted vector of FuzzyMatch results for visible + enabled
    ///         children (empty if none found).
    /// @throws std::bad_alloc if a result vector cannot be allocated.
    std::vector<FuzzyMatch> fuzzy_find_subcommands(
        const BranchCommand &parent,
        std::string_view input,
        int max_distance = 3,
        Visibility mode = Visibility::Both,
        std::vector<FuzzyMatch> *disabled_out = nullptr);

    /// @brief Sorted list of all visible + enabled subcommand names under @p cmd.
    /// @param cmd  Parent branch command.
    /// @param mode Visibility filter (default Both).
    /// @return Alphabetically sorted name list.
    std::vector<std::string> list_subcommands(
        const BranchCommand &cmd, Visibility mode = Visibility::Both);

}  // namespace pjh::cli

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
    /// @throws Any exception propagated by the user-supplied `enabled` predicate.
    inline bool is_visible_and_enabled(const BaseCommand &cmd, Visibility mode)
    {
        if (!cmd.is_enabled())
            return false;
        if ((cmd.visibility() & mode) == Visibility::Hidden)
            return false;
        return true;
    }

    /// @brief One option reachable from a command, with shadow flags.
    ///
    /// `long_shadowed`/`short_shadowed` mark a name component claimed by a
    /// nearer command, so the display layer can blank it and keep the option
    /// discoverable under its remaining spelling.
    struct ChainOption
    {
        const OptionDef *opt = nullptr;  ///< Option in the command tree.
        bool long_shadowed = false;      ///< A nearer command declares this long name.
        bool short_shadowed = false;     ///< A nearer command claims this short char.
    };

    /// @brief Options reachable from @p cmd, nearest declaration first.
    ///
    /// Walks @p cmd and its ancestors iteratively.  Long names and short chars
    /// are tracked independently (the parser resolves them independently), so a
    /// partially shadowed ancestor option is returned with the shadowed
    /// component flagged.  Options with no visible component are skipped.
    ///
    /// When @p include_current is false the current node's entries are not
    /// returned but still seed shadow tracking (used by collect_help, which
    /// lists the current node separately).
    ///
    /// @param cmd              Command to start from.
    /// @param include_current  Emit @p cmd's own options (default true).
    /// @return Entries ordered current→root; empty when no options exist.
    inline std::vector<ChainOption> collect_options_in_chain(
        const BaseCommand &cmd, bool include_current = true)
    {
        std::size_t total = 0;
        for (const BaseCommand *cur = &cmd; cur != nullptr; cur = cur->parent())
            total += cur->options().size();
        std::vector<ChainOption> out;
        std::vector<std::string_view> seen_long;
        std::vector<char> seen_short;
        out.reserve(total);
        seen_long.reserve(total);
        seen_short.reserve(total);
        bool first = true;
        for (const BaseCommand *cur = &cmd; cur != nullptr; cur = cur->parent())
        {
            for (const auto &opt_ptr : cur->options())
            {
                const auto &opt = *opt_ptr;
                const std::string_view long_name = opt.long_name();
                const char short_name = opt.short_name();
                const bool long_shadowed =
                    !long_name.empty() &&
                    std::ranges::find(seen_long, long_name) != seen_long.end();
                const bool short_shadowed =
                    short_name != 0 &&
                    std::ranges::find(seen_short, short_name) != seen_short.end();
                const bool long_visible = !long_name.empty() && !long_shadowed;
                const bool short_visible = short_name != 0 && !short_shadowed;
                if (long_visible)
                    seen_long.push_back(long_name);
                if (short_visible)
                    seen_short.push_back(short_name);
                if (!long_visible && !short_visible)
                    continue;
                if (include_current || !first)
                    out.push_back(
                        {&opt, !long_visible && !long_name.empty(),
                         !short_visible && short_name != 0});
            }
            first = false;
        }
        return out;
    }

}  // namespace pjh::cli::detail

#endif  // INCLUDE_PJH_CLI_COMMAND_MATCHER_HPP
