#include <algorithm>
#include <cstddef>
#include <format>
#include <pjh_cli/command/base_command.hpp>
#include <pjh_cli/command/branch_command.hpp>
#include <pjh_cli/detail/command_utils.hpp>
#include <pjh_cli/detail/tokenizer.hpp>
#include <pjh_cli/format/info.hpp>
#include <pjh_cli/format/matcher.hpp>
#include <string>
#include <string_view>
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

    /// @brief Split @p text into whitespace-separated tokens.
    ///
    /// Double-quoted spans are kept as one token and the quotes are stripped.
    /// Empty tokens are dropped; this scan only needs the complete tokens that
    /// precede the token under the cursor.
    ///
    /// @param text  Input slice.
    /// @return Non-empty tokens in order.
    std::vector<std::string_view> split_tokens(std::string_view text)
    {
        std::vector<std::string_view> out;
        std::size_t i = 0;
        while (i < text.size())
        {
            while (i < text.size() && text[i] == ' ') ++i;
            if (i >= text.size())
                break;
            std::size_t start = i;
            if (text[i] == '"')
            {
                start = ++i;
                while (i < text.size() && text[i] != '"') ++i;
                out.push_back(text.substr(start, i - start));
                if (i < text.size())
                    ++i;
            }
            else
            {
                while (i < text.size() && text[i] != ' ') ++i;
                out.push_back(text.substr(start, i - start));
            }
        }
        return out;
    }

    /// @brief Resolve a long option on @p cmd or its nearest ancestor.
    /// @param cmd   Command to start from.
    /// @param name  Long option name without the `--` prefix.
    /// @return Matching option, or nullptr.
    const pjh::cli::OptionDef *find_option_by_long_in_chain(
        const pjh::cli::BaseCommand &cmd, std::string_view name)
    {
        for (const auto *cur = &cmd; cur != nullptr; cur = cur->parent())
            if (const auto *opt = cur->find_option_by_long(name))
                return opt;
        return nullptr;
    }

    /// @brief Resolve a short option on @p cmd or its nearest ancestor.
    /// @param cmd  Command to start from.
    /// @param c    Short option character.
    /// @return Matching option, or nullptr.
    const pjh::cli::OptionDef *find_option_by_short_in_chain(
        const pjh::cli::BaseCommand &cmd, char c)
    {
        for (const auto *cur = &cmd; cur != nullptr; cur = cur->parent())
            if (const auto *opt = cur->find_option_by_short(c))
                return opt;
        return nullptr;
    }

    /// @brief Interpret one complete option token, setting @p pending when the
    ///        option expects its value in the following token.
    /// @param command  Command in scope for option lookup.
    /// @param token    Complete token starting with '-'.
    /// @param pending  Out-parameter: option awaiting a separate value token.
    void scan_option_token(
        const pjh::cli::BaseCommand &command,
        std::string_view token,
        const pjh::cli::OptionDef *&pending)
    {
        if (token.size() >= 2 && token[0] == '-' && token[1] == '-')
        {
            auto lo = pjh::cli::detail::Tokenizer::parse_long_option(token);
            const auto *opt = find_option_by_long_in_chain(command, lo.name);
            if (!opt && lo.is_negation)
                opt = find_option_by_long_in_chain(command, lo.negated_name);
            if (opt && !lo.has_equals && opt->has_value())
                pending = opt;
            return;
        }

        // Short token: grouped flags and compact values, mirroring consume_short.
        for (std::size_t i = 1; i < token.size(); ++i)
        {
            const auto *opt = find_option_by_short_in_chain(command, token[i]);
            if (!opt)
                return;
            if (opt->has_value())
            {
                if (i + 1 == token.size())
                    pending = opt;  // value arrives as the next token.
                return;             // otherwise the remainder is a compact value.
            }
        }
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
        while (start > 0 && line[start - 1] != ' ') --start;
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
                scan_option_token(*scan.command, t, scan.value_option);
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
            const auto *opt = find_option_by_long_in_chain(*scan.command, lo.name);
            if (!opt && lo.is_negation)
                opt = find_option_by_long_in_chain(*scan.command, lo.negated_name);
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
                const auto *opt = find_option_by_short_in_chain(*scan.command, token[i]);
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

        if (const auto *branch = cmd.as_branch())
        {
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

    std::vector<CompletionCandidate> complete_value_candidates(
        const OptionDef &opt, std::string_view prefix)
    {
        std::vector<CompletionCandidate> out;
        const auto &fn = opt.completer_fn();
        if (!fn)
            return out;

        for (const auto &c : fn())
            if (c.starts_with(prefix))
                out.push_back({c});

        std::ranges::stable_sort(out, {}, &CompletionCandidate::display);
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
