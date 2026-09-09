#include <memory>
#include <pjh_cli/core/error.hpp>
#include <pjh_cli/format/matcher.hpp>
#include <pjh_cli/parse/parse_context_writer.hpp>
#include <pjh_cli/parse/subcommand_resolver.hpp>

namespace pjh::cli
{
    /// @brief Find a subcommand by name, trying exact then fuzzy match.
    ///
    /// Exact name match is attempted first (including aliases).  If the
    /// exact match is disabled, @p out_disabled is set to true and nullptr
    /// is returned.  When no exact match is found and @p max_fuzzy_distance
    /// > 0, fuzzy_find_subcommands() is used.
    BaseCommand *SubcommandResolver::find_subcommand_match(
        BranchCommand &cmd,
        std::string_view name,
        int max_fuzzy_distance,
        bool &out_disabled)
    {
        auto *exact = cmd.find_subcommand(name);
        if (exact)
        {
            if (exact->is_enabled())
                return exact;
            out_disabled = true;
            return nullptr;
        }

        if (max_fuzzy_distance > 0)
        {
            auto fuzzy =
                fuzzy_find_subcommands(cmd, name, max_fuzzy_distance, Visibility::Both);
            if (fuzzy.size() == 1)
                return fuzzy[0].command;
        }

        return nullptr;
    }

    /// @brief Build an unknown-command error with fuzzy suggestions.
    CliError SubcommandResolver::unknown_subcommand(
        BranchCommand &cmd, std::string_view input)
    {
        std::vector<std::string> suggestions;
        for (const auto &match : fuzzy_find_subcommands(cmd, input, 3, Visibility::Both))
            suggestions.push_back(match.command->name());
        return ErrorFactory::unknown_command(input, suggestions);
    }

    /// @brief If the token matches a (possibly fuzzy) subcommand, descend.
    ///
    /// Guards against descent when @p double_dash is true or the current
    /// command is not a branch.  On success, creates a child ParseContext
    /// with parent linking and returns the matched command.
    CliResult<SubcommandResolver::SubcommandResult>
    SubcommandResolver::try_descend_subcommand(
        BaseCommand *cmd,
        ParseContext &ctx,
        std::string_view a,
        int max_fuzzy_distance,
        bool double_dash)
    {
        if (double_dash || !cmd->is_branch())
            return CliResult<SubcommandResult>::Ok(SubcommandResult{});

        auto *branch = cmd->as_branch();
        bool disabled = false;
        auto *sub = find_subcommand_match(*branch, a, max_fuzzy_distance, disabled);

        if (disabled)
            return CliResult<SubcommandResult>::Err(ErrorFactory::command_disabled(a));

        if (!sub)
            return CliResult<SubcommandResult>::Ok(SubcommandResult{});

        ParseContext child_ctx;
        ParseContextWriter::set_parent(child_ctx, std::make_shared<ParseContext>(std::move(ctx)));
        SubcommandResult r;
        r.matched = true;
        r.cmd = sub;
        r.ctx = std::move(child_ctx);
        return CliResult<SubcommandResult>::Ok(std::move(r));
    }
}  // namespace pjh::cli
