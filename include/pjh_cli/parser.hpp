#ifndef INCLUDE_PJH_CLI_PARSER_HPP
#define INCLUDE_PJH_CLI_PARSER_HPP

#include <span>
#include <string_view>
#include <vector>

#include "command/branch_command.hpp"
#include "parse_context.hpp"
#include "type.hpp"

namespace pjh::cli
{

    class Parser
    {
    public:
        Parser() = delete;

        static CliResult<ParseContext> parse_command(
            BaseCommand &root,
            std::span<const std::string_view> args,
            int max_fuzzy_distance = 0);

        static CliResult<ParseContext> parse_command(
            BaseCommand &root, int argc, char **argv, int max_fuzzy_distance = 0)
        {
            std::vector<std::string_view> args;
            args.reserve(static_cast<size_t>(argc) - 1);
            for (int a = 1; a < argc; a++) args.emplace_back(argv[a]);
            return parse_command(root, args, max_fuzzy_distance);
        }

    private:
        struct SubcommandResult
        {
            bool matched = false;
            bool disabled = false;
            BaseCommand *cmd = nullptr;
            ParseContext ctx;
        };

        static CliResult<void> apply_arg_value(
            ParseContext &ctx, size_t hash, ValueTag tag, std::string_view s);

        static CliResult<void> consume_long(
            const BaseCommand &cmd,
            ParseContext &ctx,
            std::string_view arg,
            size_t &i,
            std::span<const std::string_view> args);

        static CliResult<void> consume_short(
            const BaseCommand &cmd,
            ParseContext &ctx,
            std::string_view arg,
            size_t &i,
            std::span<const std::string_view> args);

        static void apply_flag(const OptionDef *opt, ParseContext &ctx);

        static CliResult<ParseContext> finish_parse(BaseCommand *cmd, ParseContext ctx);

        static BaseCommand *find_subcommand_match(
            BranchCommand &cmd,
            std::string_view name,
            int max_fuzzy_distance,
            bool &out_disabled);

        static pjh::result::Option<ParseContext> try_handle_help(
            BaseCommand *cmd, ParseContext &&ctx, std::string_view a, bool double_dash);

        static CliResult<SubcommandResult> try_descend_subcommand(
            BaseCommand *cmd,
            ParseContext &ctx,
            std::string_view a,
            int max_fuzzy_distance,
            bool double_dash);

        static CliResult<void> handle_extra_arg(
            const BaseCommand *cmd, ParseContext &ctx, std::string_view a, size_t pos);
    };

}  // namespace pjh::cli

#endif
