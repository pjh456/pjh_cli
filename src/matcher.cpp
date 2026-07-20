#include <algorithm>
#include <pjh_cli/command/branch_command.hpp>
#include <pjh_cli/command/leaf_command.hpp>
#include <pjh_cli/detail/command_utils.hpp>
#include <pjh_cli/matcher.hpp>
#include <sstream>
#include <vector>

namespace pjh::cli
{
    int edit_distance(std::string_view a, std::string_view b) noexcept
    {
        auto m = a.size();
        auto n = b.size();

        std::vector<int> prev(n + 1);
        std::vector<int> cur(n + 1);

        for (size_t j = 0; j <= n; j++) prev[j] = static_cast<int>(j);

        for (size_t i = 1; i <= m; i++)
        {
            cur[0] = static_cast<int>(i);
            for (size_t j = 1; j <= n; j++)
            {
                int cost = (a[i - 1] == b[j - 1]) ? 0 : 1;
                cur[j] = std::min({prev[j] + 1, cur[j - 1] + 1, prev[j - 1] + cost});
            }
            swap(prev, cur);
        }

        return prev[n];
    }

    std::vector<FuzzyMatch> fuzzy_find_subcommands(
        BranchCommand &parent, std::string_view input, int max_distance, Visibility mode)
    {
        std::vector<FuzzyMatch> results;

        for (auto &sub_ptr : parent.subcommands())
        {
            if (!detail::is_visible_and_enabled(*sub_ptr, mode))
                continue;
            int d = edit_distance(input, sub_ptr->name());
            if (d <= max_distance)
                results.push_back({sub_ptr.get(), d});
        }

        std::ranges::stable_sort(results, {}, &FuzzyMatch::distance);
        return results;
    }

    std::vector<std::string> list_subcommands(const BranchCommand &cmd, Visibility mode)
    {
        std::vector<std::string> names;
        for (const auto &sub_ptr : cmd.subcommands())
        {
            if (!detail::is_visible_and_enabled(*sub_ptr, mode))
                continue;
            names.push_back(sub_ptr->name());
        }
        return names;
    }

    std::vector<std::string> complete(
        const BaseCommand &cmd, std::string_view prefix, Visibility mode)
    {
        std::vector<std::string> candidates;

        // Subcommand name completion (branch only)
        if (!cmd.is_leaf())
        {
            auto &branch = static_cast<const BranchCommand &>(cmd);
            for (const auto &sub_ptr : branch.subcommands())
            {
                if (!detail::is_visible_and_enabled(*sub_ptr, mode))
                    continue;
                if (sub_ptr->name().starts_with(prefix))
                    candidates.push_back(sub_ptr->name());
            }
        }

        // Option completion (prefix starts with '-')
        if (!prefix.empty() && prefix[0] == '-')
        {
            if (prefix.size() >= 2 && prefix[1] == '-')
            {
                auto opt_prefix = prefix.substr(2);
                for (const auto &opt_ptr : cmd.options())
                    if (opt_ptr->long_name().starts_with(opt_prefix))
                        candidates.push_back(std::format("--{}", opt_ptr->long_name()));
            }
            else if (prefix.size() == 1)
            {
                for (const auto &opt_ptr : cmd.options())
                    if (opt_ptr->short_name() != 0)
                        candidates.push_back(std::format("-{}", opt_ptr->short_name()));
            }
            else
            {
                char c = prefix[1];
                for (const auto &opt_ptr : cmd.options())
                    if (opt_ptr->short_name() == c)
                        candidates.push_back(std::format("-{}", opt_ptr->short_name()));
            }
        }

        std::ranges::stable_sort(candidates);
        auto [first, last] = std::ranges::unique(candidates);
        candidates.erase(first, last);

        return candidates;
    }

    std::string format_usage(const BaseCommand &cmd, std::string_view program_name)
    {
        std::ostringstream os;
        os << "Usage: ";

        if (!program_name.empty())
            os << program_name;
        else if (!cmd.name().empty())
            os << cmd.name();
        else
            os << "app";

        for (const auto &opt_ptr : cmd.options())
            os << " [" << detail::option_left_label(*opt_ptr, "|") << "]";

        // Positional args (leaf only)
        if (auto *leaf = cmd.as_leaf())
        {
            for (const auto &arg : leaf->args())
            {
                if (arg.m_required)
                    os << " <" << arg.m_name << ">";
                else
                    os << " [" << arg.m_name << "]";
            }
        }

        // Subcommands (branch only)
        if (!cmd.is_leaf())
        {
            auto &branch = static_cast<const BranchCommand &>(cmd);
            auto &subcommands = branch.subcommands();
            bool has_visible = std::any_of(
                subcommands.begin(), subcommands.end(), [](const auto &sub_ptr)
                { return detail::is_visible_and_enabled(*sub_ptr, Visibility::Both); });
            if (has_visible)
                os << " <command>";
        }

        return os.str();
    }

    static void append_help_line(
        std::ostringstream &os, std::string left, std::string right, size_t left_width)
    {
        os << "  " << left;
        if (left.size() < left_width)
            os << std::string(left_width - left.size(), ' ');
        os << "  " << right << "\n";
    }

    std::string format_help(const BaseCommand &cmd, std::string_view program_name)
    {
        std::ostringstream os;

        os << format_usage(cmd, program_name) << "\n\n";

        if (!cmd.description().empty())
            os << cmd.description() << "\n\n";

        // Options
        if (!cmd.options().empty())
        {
            os << "Options:\n";

            size_t max_left = 0;
            for (const auto &opt_ptr : cmd.options())
                max_left = std::max(max_left, detail::option_left_label(*opt_ptr).size());
            size_t left_width = std::min(max_left, size_t(32));

            for (const auto &opt_ptr : cmd.options())
            {
                std::string right = opt_ptr->description();
                if (opt_ptr->is_required())
                    right += " (required)";
                append_help_line(
                    os, detail::option_left_label(*opt_ptr), right, left_width);
            }
            os << "\n";
        }

        // Positional args (leaf only)
        if (auto *leaf = cmd.as_leaf(); leaf && !leaf->args().empty())
        {
            os << "Arguments:\n";
            size_t max_left = 0;
            for (const auto &arg : leaf->args())
                max_left = std::max(max_left, arg.m_name.size());
            size_t left_width = std::min(max_left, size_t(28));

            for (const auto &arg : leaf->args())
            {
                std::string right = arg.m_description;
                if (arg.m_required)
                    right += " (required)";
                append_help_line(os, arg.m_name, right, left_width);
            }
            os << "\n";
        }

        // Subcommands (branch only)
        if (!cmd.is_leaf())
        {
            auto &branch = static_cast<const BranchCommand &>(cmd);
            auto &subcommands = branch.subcommands();

            size_t max_left = 0;
            size_t visible_count = 0;
            for (const auto &sub_ptr : subcommands)
            {
                if (!detail::is_visible_and_enabled(*sub_ptr, Visibility::Both))
                    continue;
                visible_count++;
                max_left = std::max(max_left, sub_ptr->name().size());
            }

            if (visible_count > 0)
            {
                os << "Subcommands:\n";
                size_t left_width = std::min(max_left, size_t(28));
                for (const auto &sub_ptr : subcommands)
                {
                    if (!detail::is_visible_and_enabled(*sub_ptr, Visibility::Both))
                        continue;
                    append_help_line(
                        os, sub_ptr->name(), sub_ptr->description(), left_width);
                }
            }
        }

        return os.str();
    }

}  // namespace pjh::cli
