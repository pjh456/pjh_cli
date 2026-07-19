#ifndef INCLUDE_PJH_CLI_MATCHER_HPP
#define INCLUDE_PJH_CLI_MATCHER_HPP

#include <string>
#include <string_view>
#include <vector>

#include "command.hpp"

namespace pjh::cli
{
    /// @brief Compute edit distance (Levenshtein) between two strings.
    int edit_distance(std::string_view a, std::string_view b) noexcept;

    /// @brief A fuzzy match result.
    struct FuzzyMatch
    {
        const Command *command;
        int distance;
    };

    /// @brief Find subcommands whose name fuzzily matches input.
    std::vector<FuzzyMatch> fuzzy_find_subcommands(
        const Command &parent,
        std::string_view input,
        int max_distance = 3,
        Visibility mode = Visibility::Both);

    /// @brief Sorted list of all subcommand names (respecting visibility + enabled).
    std::vector<std::string> list_subcommands(
        const Command &cmd, Visibility mode = Visibility::Both);

    /// @brief Completion candidates for a partial token on a command.
    std::vector<std::string> complete(
        const Command &cmd, std::string_view prefix, Visibility mode = Visibility::Both);

    /// @brief Format usage line, e.g. "app [--port N] <source> <dest>".
    std::string format_usage(const Command &cmd, std::string_view program_name = "");

    /// @brief Format full help block (usage + description + options + args +
    /// subcommands).
    std::string format_help(const Command &cmd, std::string_view program_name = "");

}  // namespace pjh::cli

#endif  // INCLUDE_PJH_CLI_MATCHER_HPP
