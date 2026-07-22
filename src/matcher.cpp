#include <algorithm>
#include <cstddef>
#include <format>
#include <pjh_cli/command/base_command.hpp>
#include <pjh_cli/command/branch_command.hpp>
#include <pjh_cli/detail/command_utils.hpp>
#include <pjh_cli/format/info.hpp>
#include <pjh_cli/format/matcher.hpp>
#include <string>
#include <string_view>
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
            int best_d = edit_distance(input, sub_ptr->name());
            for (const auto &a : sub_ptr->aliases())
            {
                int d = edit_distance(input, a);
                if (d < best_d)
                    best_d = d;
            }
            if (best_d <= max_distance)
                results.push_back({sub_ptr.get(), best_d});
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

    std::vector<CompletionCandidate> complete_candidates(
        const BaseCommand &cmd, std::string_view prefix, Visibility mode)
    {
        std::vector<CompletionCandidate> candidates;

        if (!cmd.is_leaf())
        {
            auto &branch = static_cast<const BranchCommand &>(cmd);
            for (const auto &sub_ptr : branch.subcommands())
                {
                    if (!detail::is_visible_and_enabled(*sub_ptr, mode))
                        continue;
                    if (sub_ptr->name().starts_with(prefix))
                        candidates.push_back({std::string(sub_ptr->name())});
                    for (const auto &a : sub_ptr->aliases())
                        if (a.starts_with(prefix))
                            candidates.push_back({a});
                }
        }

        if (!prefix.empty() && prefix[0] == '-')
        {
            if (prefix.size() >= 2 && prefix[1] == '-')
            {
                auto opt_prefix = prefix.substr(2);
                for (const auto &opt_ptr : cmd.options())
                    if (opt_ptr->long_name().starts_with(opt_prefix))
                        candidates.push_back(
                            {std::format("--{}", opt_ptr->long_name())});
            }
            else if (prefix.size() == 1)
            {
                for (const auto &opt_ptr : cmd.options())
                    if (opt_ptr->short_name() != 0)
                        candidates.push_back(
                            {std::format("-{}", opt_ptr->short_name())});
            }
            else
            {
                char c = prefix[1];
                for (const auto &opt_ptr : cmd.options())
                    if (opt_ptr->short_name() == c)
                        candidates.push_back(
                            {std::format("-{}", opt_ptr->short_name())});
            }
        }

        std::ranges::stable_sort(candidates, {}, &CompletionCandidate::display);
        auto [first, last] = std::ranges::unique(
            candidates, {}, &CompletionCandidate::display);
        candidates.erase(first, last);

        return candidates;
    }

    std::vector<std::string> complete(
        const BaseCommand &cmd, std::string_view prefix, Visibility mode)
    {
        auto ccs = complete_candidates(cmd, prefix, mode);
        std::vector<std::string> out;
        out.reserve(ccs.size());
        for (auto &cc : ccs)
            out.push_back(std::move(cc.display));
        return out;
    }

}  // namespace pjh::cli
