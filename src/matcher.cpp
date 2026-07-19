#include <algorithm>
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
        const Command &parent, std::string_view input, int max_distance, Visibility mode)
    {
        std::vector<FuzzyMatch> results;

        for (const auto &sub : parent.subcommands())
        {
            if (!detail::is_visible_and_enabled(sub, mode))
                continue;
            int d = edit_distance(input, sub.name());
            if (d <= max_distance)
                results.push_back({&sub, d});
        }

        std::ranges::stable_sort(results, {}, &FuzzyMatch::distance);

        return results;
    }

    std::vector<std::string> list_subcommands(const Command &cmd, Visibility mode)
    {
        std::vector<std::string> names;
        for (const auto &sub : cmd.subcommands())
        {
            if (!detail::is_visible_and_enabled(sub, mode))
                continue;
            names.push_back(sub.name());
        }
        return names;
    }

    std::vector<std::string> complete(
        const Command &cmd, std::string_view prefix, Visibility mode)
    {
        std::vector<std::string> candidates;

        // Subcommand name completion
        for (const auto &sub : cmd.subcommands())
        {
            if (!detail::is_visible_and_enabled(sub, mode))
                continue;
            if (sub.name().starts_with(prefix))
                candidates.push_back(sub.name());
        }

        // Option completion (prefix starts with '-')
        if (!prefix.empty() && prefix[0] == '-')
        {
            if (prefix.size() >= 2 && prefix[1] == '-')
            {
                // Long option: --... or --
                auto opt_prefix = prefix.substr(2);
                for (const auto &opt : cmd.options())
                    if (opt.long_name().starts_with(opt_prefix))
                        candidates.push_back(std::format("--{}", opt.long_name()));
            }
            else if (prefix.size() == 1)
            {
                // Bare "-": list all short options
                for (const auto &opt : cmd.options())
                    if (opt.short_name() != 0)
                        candidates.push_back(std::format("-{}", opt.short_name()));
            }
            else
            {
                // "-x" or "-xyz": match the first char
                char c = prefix[1];
                for (const auto &opt : cmd.options())
                    if (opt.short_name() == c)
                        candidates.push_back(std::format("-{}", opt.short_name()));
            }
        }

        std::ranges::stable_sort(candidates);
        auto [first, last] = std::ranges::unique(candidates);
        candidates.erase(first, last);

        return candidates;
    }

    std::string format_usage(const Command &cmd, std::string_view program_name)
    {
        std::ostringstream os;
        os << "Usage: ";

        if (!program_name.empty())
            os << program_name;
        else if (!cmd.name().empty())
            os << cmd.name();
        else
            os << "app";

        // Options summary
        for (const auto &opt : cmd.options())
            os << " [" << detail::option_left_label(opt, "|") << "]";

        // Positional args
        for (const auto &arg : cmd.args())
        {
            if (arg.m_required)
                os << " <" << arg.m_name << ">";
            else
                os << " [" << arg.m_name << "]";
        }

        // Subcommands (only if no args left to consume)
        if (!cmd.subcommands().empty())
        {
            bool has_visible = std::any_of(
                cmd.subcommands().begin(), cmd.subcommands().end(), [](const Command &sub)
                { return detail::is_visible_and_enabled(sub, Visibility::Both); });
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

    std::string format_help(const Command &cmd, std::string_view program_name)
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
            for (const auto &opt : cmd.options())
                max_left = std::max(max_left, detail::option_left_label(opt).size());
            size_t left_width = std::min(max_left, size_t(32));

            for (const auto &opt : cmd.options())
            {
                std::string right = opt.description();
                if (opt.is_required())
                    right += " (required)";
                append_help_line(os, detail::option_left_label(opt), right, left_width);
            }
            os << "\n";
        }

        // Positional args
        if (!cmd.args().empty())
        {
            os << "Arguments:\n";
            size_t max_left = 0;
            for (const auto &arg : cmd.args())
                max_left = std::max(max_left, arg.m_name.size());
            size_t left_width = std::min(max_left, size_t(28));

            for (const auto &arg : cmd.args())
            {
                std::string right = arg.m_description;
                if (arg.m_required)
                    right += " (required)";
                append_help_line(os, arg.m_name, right, left_width);
            }
            os << "\n";
        }

        // Subcommands
        size_t max_left = 0;
        size_t visible_count = 0;
        for (const auto &sub : cmd.subcommands())
        {
            if (!detail::is_visible_and_enabled(sub, Visibility::Both))
                continue;
            visible_count++;
            max_left = std::max(max_left, sub.name().size());
        }

        if (visible_count > 0)
        {
            os << "Subcommands:\n";
            size_t left_width = std::min(max_left, size_t(28));
            for (const auto &sub : cmd.subcommands())
            {
                if (!detail::is_visible_and_enabled(sub, Visibility::Both))
                    continue;
                append_help_line(os, sub.name(), sub.description(), left_width);
            }
        }

        return os.str();
    }

}  // namespace pjh::cli
