#ifndef INCLUDE_PJH_CLI_HELP_NAVIGATOR_HPP
#define INCLUDE_PJH_CLI_HELP_NAVIGATOR_HPP

#include <pjh_cli/command/base_command.hpp>
#include <pjh_cli/command/branch_command.hpp>
#include <pjh_cli/format/info.hpp>
#include <string>
#include <string_view>
#include <vector>

namespace pjh::cli
{

    enum class HelpNavigationKind
    {
        RootHelp,         ///< Show root help (tokens.size() == 1)
        SubcommandHelp,   ///< Navigated to a resolved subcommand
        NonBranch,        ///< Descended into a leaf — no subcommands
        UnknownCommand,   ///< Subcommand not found at current level
    };

    struct HelpNavigationResult
    {
        HelpNavigationKind kind = HelpNavigationKind::RootHelp;

        /// Resolved command for RootHelp / SubcommandHelp.
        const BaseCommand *resolved = nullptr;

        /// Command name for NonBranch.
        std::string failed_command_name;

        /// Unrecognised token for UnknownCommand.
        std::string failed_token;

        /// Fuzzy suggestions for UnknownCommand.
        SuggestionInfo suggestions;
    };

    class HelpNavigator
    {
    public:
        HelpNavigator() = delete;

        static HelpNavigationResult navigate(
            const BranchCommand &root,
            const std::vector<std::string> &tokens);
    };

    class HelpNavigationOutput
    {
    public:
        HelpNavigationOutput() = delete;

        static std::string format(const HelpNavigationResult &result);

        static std::string format(
            const BranchCommand &root,
            const std::vector<std::string> &tokens);
    };

} // namespace pjh::cli

#endif