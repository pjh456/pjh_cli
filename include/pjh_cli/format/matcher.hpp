#ifndef INCLUDE_PJH_CLI_MATCHER_HPP
#define INCLUDE_PJH_CLI_MATCHER_HPP

#include <pjh_cli/command/base_command.hpp>
#include <pjh_cli/command/branch_command.hpp>
#include <string>
#include <string_view>
#include <vector>

namespace pjh::cli
{
    /// @brief Compute Levenshtein edit distance between two strings.
    /// @param a First string.
    /// @param b Second string.
    /// @return Number of single-character edits (insert/delete/substitute) needed.
    int edit_distance(std::string_view a, std::string_view b) noexcept;

    /// @brief A fuzzy match result returned by fuzzy_find_subcommands().
    struct FuzzyMatch
    {
        BaseCommand *command;  ///< The matched subcommand.
        int distance;          ///< Levenshtein distance (lower = closer).
    };

    /// @brief Find subcommands whose name fuzzily matches @p input.
    ///
    /// Enumerates all direct children of @p parent, applies the visibility
    /// + enabled filter, then computes edit_distance() against each name.
    /// Results within @p max_distance are returned sorted by distance.
    ///
    /// @param parent       Parent branch to search.
    /// @param input        User input (potentially misspelled).
    /// @param max_distance  Max edit distance to accept (default 3).
    /// @param mode         Visibility filter (default Both).
    /// @return Sorted vector of FuzzyMatch results (empty if none found).
    std::vector<FuzzyMatch> fuzzy_find_subcommands(
        BranchCommand &parent,
        std::string_view input,
        int max_distance = 3,
        Visibility mode = Visibility::Both);

    /// @brief Sorted list of all visible + enabled subcommand names under @p cmd.
    /// @param cmd  Parent branch command.
    /// @param mode Visibility filter (default Both).
    /// @return Alphabetically sorted name list.
    std::vector<std::string> list_subcommands(
        const BranchCommand &cmd, Visibility mode = Visibility::Both);

    /// @brief Completion candidates for a partial token on a command.
    ///
    /// Matches subcommand names (when @p prefix does not start with '-') or
    /// option names (when @p prefix starts with '-').  Option completion
    /// handles long options (--ver→ --verbose) and short options (-v).
    /// Results are sorted and deduplicated.
    ///
    /// @param cmd    Current command whose options/subcommands are consulted.
    /// @param prefix Partial token to match against.
    /// @param mode   Visibility filter (default Both).
    /// @return Sorted, deduplicated candidate list (may be empty).
    std::vector<std::string> complete(
        const BaseCommand &cmd,
        std::string_view prefix,
        Visibility mode = Visibility::Both);

}  // namespace pjh::cli

#endif
