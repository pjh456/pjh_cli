#include <pjh_cli/matcher.hpp>

#include <algorithm>
#include <sstream>
#include <vector>

namespace pjh::cli
{
    int edit_distance(
        std::string_view a,
        std::string_view b)
    {
        auto m = a.size();
        auto n = b.size();

        std::vector<int> prev(n + 1);
        std::vector<int> cur(n + 1);

        for (size_t j = 0; j <= n; j++)
            prev[j] = static_cast<int>(j);

        for (size_t i = 1; i <= m; i++)
        {
            cur[0] = static_cast<int>(i);
            for (size_t j = 1; j <= n; j++)
            {
                int cost = (a[i - 1] == b[j - 1]) ? 0 : 1;
                cur[j] = std::min(
                    {prev[j] + 1,
                     cur[j - 1] + 1,
                     prev[j - 1] + cost});
            }
            swap(prev, cur);
        }

        return prev[n];
    }

    std::vector<FuzzyMatch>
    fuzzy_find_subcommands(
        const Command &parent,
        std::string_view input,
        int max_distance,
        Visibility mode)
    {
        std::vector<FuzzyMatch> results;

        for (const auto &sub : parent.subcommands())
        {
            if (!sub.is_enabled())
                continue;
            if ((sub.visibility() & mode) == Visibility::Hidden)
                continue;

            int d = edit_distance(input, sub.name());
            if (d <= max_distance)
                results.push_back({&sub, d});
        }

        std::stable_sort(
            results.begin(),
            results.end(),
            [](const FuzzyMatch &a, const FuzzyMatch &b)
            {
                return a.distance < b.distance;
            });

        return results;
    }

    std::vector<std::string>
    list_subcommands(
        const Command &cmd,
        Visibility mode)
    {
        std::vector<std::string> names;
        for (const auto &sub : cmd.subcommands())
        {
            if (!sub.is_enabled())
                continue;
            if ((sub.visibility() & mode) == Visibility::Hidden)
                continue;
            names.push_back(sub.name());
        }
        return names;
    }

    std::vector<std::string>
    complete(
        const Command &cmd,
        std::string_view prefix,
        Visibility mode)
    {
        std::vector<std::string> candidates;

        // Subcommand name completion
        for (const auto &sub : cmd.subcommands())
        {
            if (!sub.is_enabled())
                continue;
            if ((sub.visibility() & mode) == Visibility::Hidden)
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
                    if (opt.m_long_name.starts_with(opt_prefix))
                        candidates.push_back(
                            "--" + opt.m_long_name);
            }
            else if (prefix.size() == 1)
            {
                // Bare "-": list all short options
                for (const auto &opt : cmd.options())
                    if (opt.m_short_name != 0)
                        candidates.push_back(
                            std::string("-") + opt.m_short_name);
            }
            else
            {
                // "-x" or "-xyz": match the first char
                char c = prefix[1];
                for (const auto &opt : cmd.options())
                    if (opt.m_short_name == c)
                        candidates.push_back(
                            std::string("-") + opt.m_short_name);
            }
        }

        std::stable_sort(candidates.begin(), candidates.end());
        candidates.erase(
            std::unique(
                candidates.begin(),
                candidates.end()),
            candidates.end());

        return candidates;
    }

    std::string
    format_usage(
        const Command &cmd,
        std::string_view program_name)
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
        {
            os << " [";
            if (opt.m_short_name != 0)
                os << "-" << opt.m_short_name;
            if (opt.m_short_name != 0 && !opt.m_long_name.empty())
                os << "|";
            if (!opt.m_long_name.empty())
                os << "--" << opt.m_long_name;

            if (opt.m_has_value)
            {
                os << " ";
                auto label = opt.m_long_name.empty()
                                 ? std::string(1, opt.m_short_name)
                                 : opt.m_long_name;
                std::transform(
                    label.begin(), label.end(),
                    label.begin(),
                    [](unsigned char c)
                    { return static_cast<char>(std::toupper(c)); });
                os << label;
            }
            os << "]";
        }

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
            bool has_visible = false;
            for (const auto &sub : cmd.subcommands())
            {
                if (sub.is_enabled() &&
                    sub.visibility() != Visibility::Hidden)
                {
                    has_visible = true;
                    break;
                }
            }
            if (has_visible)
                os << " <command>";
        }

        return os.str();
    }

    static void
    append_help_line(
        std::ostringstream &os,
        std::string left,
        std::string right,
        size_t left_width)
    {
        os << "  " << left;
        if (left.size() < left_width)
            os << std::string(left_width - left.size(), ' ');
        os << "  " << right << "\n";
    }

    std::string
    format_help(
        const Command &cmd,
        std::string_view program_name)
    {
        std::ostringstream os;

        os << format_usage(cmd, program_name) << "\n\n";

        if (!cmd.description().empty())
            os << cmd.description() << "\n\n";

        // Options
        if (!cmd.options().empty())
        {
            os << "Options:\n";

            auto opt_left = [](const OptionDef &opt) -> std::string
            {
                std::string left;
                if (opt.m_short_name != 0)
                    left = std::string("-") + opt.m_short_name;
                if (opt.m_short_name != 0 && !opt.m_long_name.empty())
                    left += ", ";
                if (!opt.m_long_name.empty())
                    left += "--" + opt.m_long_name;
                if (opt.m_has_value)
                {
                    auto label =
                        opt.m_long_name.empty()
                            ? std::string(1, opt.m_short_name)
                            : opt.m_long_name;
                    std::transform(
                        label.begin(), label.end(),
                        label.begin(),
                        [](unsigned char c)
                        { return static_cast<char>(std::toupper(c)); });
                    left += " " + label;
                }
                return left;
            };

            size_t max_left = 0;
            for (const auto &opt : cmd.options())
                max_left = std::max(max_left, opt_left(opt).size());
            size_t left_width = std::min(max_left, size_t(32));

            for (const auto &opt : cmd.options())
            {
                std::string right = opt.m_description;
                if (opt.m_required)
                    right += " (required)";
                append_help_line(os, opt_left(opt), right, left_width);
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
        bool has_visible = false;
        size_t max_left = 0;
        for (const auto &sub : cmd.subcommands())
        {
            if (!sub.is_enabled() ||
                sub.visibility() == Visibility::Hidden)
                continue;
            has_visible = true;
            max_left = std::max(max_left, sub.name().size());
        }

        if (has_visible)
        {
            os << "Subcommands:\n";
            size_t left_width = std::min(max_left, size_t(28));
            for (const auto &sub : cmd.subcommands())
            {
                if (!sub.is_enabled() ||
                    sub.visibility() == Visibility::Hidden)
                    continue;
                append_help_line(os, sub.name(), sub.description(), left_width);
            }
        }

        return os.str();
    }

} // namespace pjh::cli
