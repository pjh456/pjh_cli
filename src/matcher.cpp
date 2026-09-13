#include <algorithm>
#include <array>
#include <cstddef>
#include <limits>
#include <pjh_cli/command/arg_scan.hpp>
#include <pjh_cli/command/base_command.hpp>
#include <pjh_cli/command/branch_command.hpp>
#include <pjh_cli/command/matcher.hpp>
#include <pjh_cli/detail/tokenizer.hpp>
#include <pjh_cli/format/info.hpp>
#include <pjh_cli/format/matcher.hpp>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace
{
    /// @brief Context resolved for the token under the cursor.
    struct CompletionScan
    {
        const pjh::cli::BaseCommand *command = nullptr;
        const pjh::cli::OptionDef *value_option = nullptr;  ///< null: name completion.
        std::string_view prefix;                            ///< token under cursor.
    };

    /// @brief Split @p text into tokens using the shared execution grammar.
    ///
    /// Delegates to @ref pjh::cli::detail::Tokenizer::tokenize so completion
    /// resolves exactly the token stream (space/tab separators, quote
    /// stripping, the narrow backslash escapes, preserved empty quoted tokens)
    /// that process_line and HintBuilder execute.  Owned strings are returned
    /// because the shared scanner merges non-contiguous spans.
    ///
    /// @param text  Input slice.
    /// @return Tokens in order.
    std::vector<std::string> split_tokens(std::string_view text)
    {
        return pjh::cli::detail::Tokenizer::tokenize(text);
    }

    /// @brief Walk the tokens before @p cursor to find the command in scope and
    ///        the option whose value is being typed.
    ///
    /// Skips option values, handles `--opt=value`, grouped short flags and
    /// compact `-pVALUE`, honours the `--` barrier, and descends subcommands.
    ///
    /// @param root    Root of the command tree.
    /// @param line    Full input line.
    /// @param cursor  Byte offset of the cursor (clamped to line.size()).
    /// @return Resolved command, optional value option, and the prefix token.
    CompletionScan scan_completion_context(
        const pjh::cli::BaseCommand &root, std::string_view line, std::size_t cursor)
    {
        if (cursor > line.size())
            cursor = line.size();

        std::size_t start = cursor;
        // The cursor token boundary uses the same separator set as the shared
        // tokenizer (space + tab) so completion and execution agree.
        while (start > 0 && !pjh::cli::detail::Tokenizer::is_separator(line[start - 1]))
            --start;
        const std::string_view token = line.substr(start, cursor - start);
        const std::string_view head = line.substr(0, start);

        CompletionScan scan;
        scan.command = &root;

        for (const auto &t : split_tokens(head))
        {
            if (t == "--")
                break;
            if (scan.value_option)
            {
                scan.value_option = nullptr;  // this token is the pending value.
                continue;
            }
            if (t.size() >= 2 && t[0] == '-')
            {
                auto info = pjh::cli::detail::scan_option_token(*scan.command, t);
                if (info.needs_next_token)
                    scan.value_option = info.option;
                continue;
            }
            if (const auto *branch = scan.command->as_branch())
                if (const auto *sub = branch->find_subcommand(t))
                    scan.command = sub;
        }

        // Interpret the token under the cursor.
        if (token.size() >= 2 && token[0] == '-' && token[1] == '-')
        {
            auto lo = pjh::cli::detail::Tokenizer::parse_long_option(token);
            const auto *opt =
                pjh::cli::detail::find_option_by_long_in_chain(*scan.command, lo.name);
            if (!opt && lo.is_negation)
                opt = pjh::cli::detail::find_option_by_long_in_chain(
                    *scan.command, lo.negated_name);
            if (opt && lo.has_equals && opt->has_value())
            {
                scan.value_option = opt;
                scan.prefix = lo.value;
                return scan;
            }
            scan.value_option = nullptr;
            scan.prefix = token;
            return scan;
        }

        if (token.size() >= 3 && token[0] == '-' && token[1] != '-')
        {
            for (std::size_t i = 1; i < token.size(); ++i)
            {
                const auto *opt = pjh::cli::detail::find_option_by_short_in_chain(
                    *scan.command, token[i]);
                if (!opt)
                    break;
                if (opt->has_value())
                {
                    if (i + 1 < token.size())
                    {
                        scan.value_option = opt;
                        scan.prefix = token.substr(i + 1);
                        return scan;
                    }
                    break;  // valued option without an attached value.
                }
            }
            scan.value_option = nullptr;
            scan.prefix = token;
            return scan;
        }

        // Word token (or bare '-' / empty): the pending option may take it.
        scan.prefix = token;
        return scan;
    }
}  // namespace

namespace pjh::cli
{
    int edit_distance(std::string_view a, std::string_view b)
    {
        auto m = a.size();
        auto n = b.size();

        // Two DP rows: the stack buffers cover candidates of at most 63
        // characters (every command name, alias, and long option in the tree),
        // so the common short-name case allocates nothing; the vectors are the
        // fallback for longer candidates and can throw std::bad_alloc.
        constexpr std::size_t kStackRow = 64;
        std::array<int, kStackRow> prev_s{};
        std::array<int, kStackRow> cur_s{};
        std::vector<int> prev_v, cur_v;
        int *prev, *cur;
        if (n + 1 <= kStackRow)
        {
            prev = prev_s.data();
            cur = cur_s.data();
        }
        else
        {
            prev_v.assign(n + 1, 0);
            cur_v.assign(n + 1, 0);
            prev = prev_v.data();
            cur = cur_v.data();
        }

        for (size_t j = 0; j <= n; j++) prev[j] = static_cast<int>(j);

        for (size_t i = 1; i <= m; i++)
        {
            cur[0] = static_cast<int>(i);
            for (size_t j = 1; j <= n; j++)
            {
                int cost = (a[i - 1] == b[j - 1]) ? 0 : 1;
                cur[j] = std::min({prev[j] + 1, cur[j - 1] + 1, prev[j - 1] + cost});
            }
            std::swap(prev, cur);
        }

        return prev[n];
    }

    std::vector<FuzzyMatch> fuzzy_find_subcommands(
        const BranchCommand &parent,
        std::string_view input,
        int max_distance,
        Visibility mode)
    {
        std::vector<FuzzyMatch> results;
        // Allocation-free upper bound: at most one match per child.
        results.reserve(parent.subcommands().size());

        for (auto &sub_ptr : parent.subcommands())
        {
            if (!detail::is_visible_and_enabled(*sub_ptr, mode))
                continue;
            int best_d = std::numeric_limits<int>::max();
            auto consider = [&](std::string_view cand)
            {
                // Length precheck: candidates whose length differs from the
                // input by more than max_distance are provably farther than
                // max_distance edits away (distance >= | |a| - |b| |), so the
                // DP is skipped for them.
                if (!detail::within_edit_distance_bound(input, cand, max_distance))
                    return;
                int d = edit_distance(input, cand);
                if (d < best_d)
                    best_d = d;
            };
            consider(sub_ptr->name());
            for (const auto &a : sub_ptr->aliases()) consider(a);
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

        if (const auto *branch = cmd.as_branch())
        {
            // Upper bound over all children (names + aliases) without invoking
            // the user enabled predicate; hidden children only waste capacity.
            std::size_t upper = 0;
            for (const auto &sub_ptr : branch->subcommands())
                upper += 1 + sub_ptr->aliases().size();
            candidates.reserve(upper);
            for (const auto &sub_ptr : branch->subcommands())
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
            const auto chain = detail::collect_options_in_chain(cmd);
            candidates.reserve(candidates.size() + chain.size());
            if (prefix.size() >= 2 && prefix[1] == '-')
            {
                auto opt_prefix = prefix.substr(2);
                for (const auto &entry : chain)
                {
                    const auto &name = entry.opt->long_name();
                    if (!entry.long_shadowed && !name.empty() &&
                        name.starts_with(opt_prefix))
                    {
                        std::string s;
                        s.reserve(name.size() + 2);
                        s += "--";
                        s += name;
                        candidates.push_back({std::move(s)});
                    }
                }
            }
            else if (prefix.size() == 1)
            {
                for (const auto &entry : chain)
                    if (!entry.short_shadowed && entry.opt->short_name() != 0)
                    {
                        std::string s{"-"};
                        s += entry.opt->short_name();
                        candidates.push_back({std::move(s)});
                    }
            }
            else
            {
                char c = prefix[1];
                for (const auto &entry : chain)
                    if (!entry.short_shadowed && entry.opt->short_name() == c)
                    {
                        std::string s{"-"};
                        s += entry.opt->short_name();
                        candidates.push_back({std::move(s)});
                    }
            }
        }

        std::ranges::sort(candidates, {}, &CompletionCandidate::display);
        auto [first, last] =
            std::ranges::unique(candidates, {}, &CompletionCandidate::display);
        candidates.erase(first, last);

        return candidates;
    }

    std::vector<std::string> complete(
        const BaseCommand &cmd, std::string_view prefix, Visibility mode)
    {
        auto ccs = complete_candidates(cmd, prefix, mode);
        std::vector<std::string> out;
        out.reserve(ccs.size());
        for (auto &cc : ccs) out.push_back(std::move(cc.display));
        return out;
    }

    std::vector<CompletionCandidate> complete_value_candidates(
        const OptionDef &opt, std::string_view prefix)
    {
        std::vector<CompletionCandidate> out;
        const auto &fn = opt.completer_fn();
        if (!fn)
            return out;

        auto items = fn();
        out.reserve(items.size());
        for (auto &c : items)
            if (c.starts_with(prefix))
                out.push_back({std::move(c)});

        std::ranges::sort(out, {}, &CompletionCandidate::display);
        auto [first, last] = std::ranges::unique(out, {}, &CompletionCandidate::display);
        out.erase(first, last);
        return out;
    }

    CompletionResult complete_line_result(
        const BaseCommand &root,
        std::string_view line,
        std::size_t cursor,
        Visibility mode)
    {
        auto scan = scan_completion_context(root, line, cursor);
        CompletionResult result;
        result.prefix_len = scan.prefix.size();
        if (scan.value_option)
            result.candidates =
                complete_value_candidates(*scan.value_option, scan.prefix);
        else
            result.candidates = complete_candidates(*scan.command, scan.prefix, mode);
        return result;
    }

    std::vector<CompletionCandidate> complete_line(
        const BaseCommand &root,
        std::string_view line,
        std::size_t cursor,
        Visibility mode)
    {
        return complete_line_result(root, line, cursor, mode).candidates;
    }

}  // namespace pjh::cli
