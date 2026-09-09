#include <pjh_cli/command/base_command.hpp>
#include <pjh_cli/command/branch_command.hpp>
#include <pjh_cli/command/matcher.hpp>
#include <pjh_cli/console/query_explorer.hpp>
#include <pjh_cli/console/query_output.hpp>
#include <pjh_cli/format/console_output.hpp>
#include <pjh_cli/format/help_formatter.hpp>
#include <pjh_cli/format/info.hpp>
#include <pjh_cli/format/matcher.hpp>
#include <string>
#include <string_view>
#include <vector>

namespace pjh::cli
{

    // ── QueryExplorer ──

    QueryResult QueryExplorer::explore(const BranchCommand &root, std::string_view query)
    {
        QueryResult result;

        if (query.empty())
        {
            result.kind = QueryKind::Listing;
            result.names = list_subcommands(root, Visibility::Repl);
            return result;
        }

        std::vector<std::string> matched;
        for (const auto &sub_ptr : root.subcommands())
        {
            if (!detail::is_visible_and_enabled(*sub_ptr, Visibility::Repl))
                continue;
            if (sub_ptr->name().find(query) != std::string_view::npos)
            {
                matched.push_back(sub_ptr->name());
                continue;
            }
            for (const auto &a : sub_ptr->aliases())
                if (a.find(query) != std::string_view::npos)
                {
                    matched.push_back(sub_ptr->name());
                    break;
                }
        }

        if (!matched.empty())
        {
            result.kind = QueryKind::Matched;
            result.names = std::move(matched);
            return result;
        }

        auto fuzzy = fuzzy_find_subcommands(root, query, 3, Visibility::Repl);
        if (!fuzzy.empty())
        {
            result.kind = QueryKind::Fuzzy;
            result.suggestions.matches.reserve(fuzzy.size());
            for (auto &f : fuzzy)
                result.suggestions.matches.push_back({f.command->name(), f.distance});
            return result;
        }

        result.kind = QueryKind::NoMatch;
        result.usage_line = HelpFormatter::format_usage(root, root.name());
        return result;
    }

    // ── QueryOutput ──

    std::string QueryOutput::format(const QueryResult &result)
    {
        switch (result.kind)
        {
        case QueryKind::Listing:
            return ConsoleOutput::format_subcommand_list(result.names);

        case QueryKind::Matched:
            return ConsoleOutput::format_matched_subcommands(result.names);

        case QueryKind::Fuzzy:
        {
            auto sug = ConsoleOutput::format_suggestions(result.suggestions);
            return "Did you mean:" + sug;
        }

        case QueryKind::NoMatch:
            return ConsoleOutput::format_no_match(result.usage_line);
        }

        return {};
    }

    std::string QueryOutput::format(const BranchCommand &root, std::string_view query)
    {
        return format(QueryExplorer::explore(root, query));
    }

    std::string QueryOutput::format(
        const BranchCommand &root,
        std::string_view query,
        const QueryFormatter &custom_fmt)
    {
        auto result = QueryExplorer::explore(root, query);
        return custom_fmt(result);
    }

}  // namespace pjh::cli