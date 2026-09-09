#include <pjh_cli/command/branch_command.hpp>
#include <pjh_cli/console/help_navigator.hpp>
#include <pjh_cli/format/console_output.hpp>
#include <pjh_cli/format/help_formatter.hpp>
#include <pjh_cli/format/info.hpp>
#include <pjh_cli/format/matcher.hpp>
#include <string>
#include <vector>

namespace pjh::cli
{

    // ── HelpNavigator ──

    HelpNavigationResult HelpNavigator::navigate(
        const BranchCommand &root,
        const std::vector<std::string> &tokens)
    {
        HelpNavigationResult result;

        if (tokens.size() == 1)
        {
            result.kind = HelpNavigationKind::RootHelp;
            result.resolved = &root;
            return result;
        }

        const BaseCommand *target = &root;
        for (size_t i = 1; i < tokens.size(); i++)
        {
            if (!target->is_branch())
            {
                result.kind = HelpNavigationKind::NonBranch;
                result.failed_command_name = target->name();
                return result;
            }

            auto *branch = target->as_branch();
            auto *sub = branch->find_subcommand(tokens[i]);
            if (!sub)
            {
                result.kind = HelpNavigationKind::UnknownCommand;
                result.failed_token = tokens[i];

                auto fuzzy =
                    fuzzy_find_subcommands(*branch, tokens[i], 3, Visibility::Repl);
                result.suggestions.matches.reserve(fuzzy.size());
                for (auto &f : fuzzy)
                    result.suggestions.matches.push_back(
                        {f.command->name(), f.distance});
                return result;
            }
            target = sub;
        }

        result.kind = HelpNavigationKind::SubcommandHelp;
        result.resolved = target;
        return result;
    }

    // ── HelpNavigationOutput ──

    std::string HelpNavigationOutput::format(const HelpNavigationResult &result)
    {
        switch (result.kind)
        {
        case HelpNavigationKind::RootHelp:
        case HelpNavigationKind::SubcommandHelp:
            return HelpFormatter::format_help(*result.resolved);

        case HelpNavigationKind::NonBranch:
            return ConsoleOutput::format_has_no_subcommands(
                       result.failed_command_name)
                   + "\n";

        case HelpNavigationKind::UnknownCommand:
        {
            auto sug = ConsoleOutput::format_suggestions(result.suggestions);
            return ConsoleOutput::format_unknown_subcommand(
                       result.failed_token, sug)
                   + "\n";
        }
        }

        return {};
    }

    std::string HelpNavigationOutput::format(
        const BranchCommand &root,
        const std::vector<std::string> &tokens)
    {
        return format(HelpNavigator::navigate(root, tokens));
    }

    std::string HelpNavigationOutput::format(
        const BranchCommand &root,
        const std::vector<std::string> &tokens,
        const HelpNavigationFormatter &custom_fmt)
    {
        auto result = HelpNavigator::navigate(root, tokens);
        return custom_fmt(result);
    }

} // namespace pjh::cli