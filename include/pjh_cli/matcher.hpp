#ifndef INCLUDE_PJH_CLI_MATCHER_HPP
#define INCLUDE_PJH_CLI_MATCHER_HPP

#include <string>
#include <string_view>
#include <vector>

#include "command/base_command.hpp"
#include "command/branch_command.hpp"

namespace pjh::cli
{
    int edit_distance(std::string_view a, std::string_view b) noexcept;

    struct FuzzyMatch
    {
        BaseCommand *command;
        int distance;
    };

    std::vector<FuzzyMatch> fuzzy_find_subcommands(
        BranchCommand &parent,
        std::string_view input,
        int max_distance = 3,
        Visibility mode = Visibility::Both);

    std::vector<std::string> list_subcommands(
        const BranchCommand &cmd, Visibility mode = Visibility::Both);

    std::vector<std::string> complete(
        const BaseCommand &cmd,
        std::string_view prefix,
        Visibility mode = Visibility::Both);

    std::string format_usage(const BaseCommand &cmd, std::string_view program_name = "");

    std::string format_help(const BaseCommand &cmd, std::string_view program_name = "");

}  // namespace pjh::cli

#endif
