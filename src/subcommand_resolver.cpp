#include <memory>
#include <pjh_cli/command/matcher.hpp>
#include <pjh_cli/core/error.hpp>
#include <pjh_cli/parse/detail/parse_context_writer.hpp>
#include <pjh_cli/parse/subcommand_resolver.hpp>
#include <string>
#include <vector>

namespace
{
    /// @brief Fixed Levenshtein distance for unknown-command suggestions.
    ///
    /// Shared by unknown_subcommand() and the suggestions_known_empty flag so
    /// the suggestion threshold and its reuse condition cannot drift apart.
    constexpr int k_suggestion_distance = 3;
}  // namespace

namespace pjh::cli
{
    /// @brief Find a subcommand by name, trying exact then fuzzy match.
    ///
    /// Exact name match is attempted first (including aliases).  If the
    /// exact match is disabled, @p out_disabled is set to the matched command
    /// and nullptr is returned.  When no exact match is found and
    /// @p max_fuzzy_distance > 0, fuzzy_find_subcommands() is used; a unique
    /// enabled candidate is returned, while several enabled candidates are
    /// appended to @p out_ambiguous.  When no enabled candidate matches but
    /// exactly one visible disabled command does, @p out_disabled is set to
    /// that command.
    BaseCommand *SubcommandResolver::find_subcommand_match(
        BranchCommand &cmd,
        std::string_view name,
        int max_fuzzy_distance,
        const BaseCommand *&out_disabled,
        std::vector<std::string> &out_ambiguous)
    {
        out_disabled = nullptr;
        auto *exact = cmd.find_subcommand(name);
        if (exact)
        {
            if (exact->is_enabled())
                return exact;
            out_disabled = exact;
            return nullptr;
        }

        if (max_fuzzy_distance > 0)
        {
            std::vector<FuzzyMatch> disabled;
            auto fuzzy = fuzzy_find_subcommands(
                cmd, name, max_fuzzy_distance, Visibility::Both, &disabled);
            if (fuzzy.size() == 1)
                return fuzzy[0].command;
            if (fuzzy.size() > 1)
            {
                for (const auto &m : fuzzy) out_ambiguous.push_back(m.command->name());
                return nullptr;
            }
            if (disabled.size() == 1)
                out_disabled = disabled[0].command;
        }

        return nullptr;
    }

    /// @brief Build an unknown-command error with fuzzy suggestions.
    CliError SubcommandResolver::unknown_subcommand(
        BranchCommand &cmd, std::string_view input)
    {
        std::vector<std::string> suggestions;
        for (const auto &match :
             fuzzy_find_subcommands(cmd, input, k_suggestion_distance, Visibility::Both))
            suggestions.push_back(match.command->name());
        return ErrorFactory::unknown_command(input, suggestions);
    }

    /// @brief If the token matches a (possibly fuzzy) subcommand, descend.
    ///
    /// Guards against descent when @p double_dash is true, the token is
    /// dash-prefixed, or the current command is not a branch.  On success,
    /// creates a child ParseContext with parent linking and returns the
    /// matched command.  Reports Err(ambiguous_command) when several fuzzy
    /// candidates are within threshold.
    CliResult<SubcommandResolver::SubcommandResult>
    SubcommandResolver::try_descend_subcommand(
        BaseCommand *cmd,
        ParseContext &ctx,
        std::string_view a,
        int max_fuzzy_distance,
        bool double_dash)
    {
        if (double_dash || a.starts_with('-') || !cmd->is_branch())
            return CliResult<SubcommandResult>::Ok(SubcommandResult{});

        auto *branch = cmd->as_branch();
        const BaseCommand *disabled = nullptr;
        std::vector<std::string> ambiguous;
        auto *sub =
            find_subcommand_match(*branch, a, max_fuzzy_distance, disabled, ambiguous);

        if (disabled)
            return CliResult<SubcommandResult>::Err(
                ErrorFactory::command_disabled(disabled->name()));

        if (!ambiguous.empty())
            return CliResult<SubcommandResult>::Err(
                ErrorFactory::ambiguous_command(a, ambiguous));

        if (!sub)
        {
            SubcommandResult r;
            r.suggestions_known_empty = max_fuzzy_distance >= k_suggestion_distance;
            return CliResult<SubcommandResult>::Ok(std::move(r));
        }

        ParseContext child_ctx;
        detail::ParseContextWriter::set_parent(
            child_ctx, std::make_shared<ParseContext>(std::move(ctx)));
        SubcommandResult r;
        r.matched = true;
        r.cmd = sub;
        r.ctx = std::move(child_ctx);
        return CliResult<SubcommandResult>::Ok(std::move(r));
    }
}  // namespace pjh::cli
