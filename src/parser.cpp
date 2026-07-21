#include <cstdlib>
#include <filesystem>
#include <format>
#include <memory>
#include <pjh_cli/command/branch_command.hpp>
#include <pjh_cli/command/leaf_command.hpp>
#include <pjh_cli/converter.hpp>
#include <pjh_cli/detail/tokenizer.hpp>
#include <pjh_cli/matcher.hpp>
#include <pjh_cli/parser.hpp>
#include <span>
#include <string>
#include <string_view>
#include <utility>

#include "pjh_cli/command/base_command.hpp"
#include "pjh_cli/error.hpp"
#include "pjh_cli/parse_context.hpp"
#include "pjh_cli/type.hpp"

namespace pjh::cli
{
    namespace
    {
        /// @brief Dispatch a raw string value for a positional arg (no virtual dispatch).
        CliResult<void> apply_arg_value(
            ParseContext &ctx, size_t hash, ValueTag tag, std::string_view s)
        {
            switch (tag)
            {
            case ValueTag::Bool:
            {
                auto r = Converter<bool>::from_string(s);
                if (r.is_err())
                    return CliResult<void>::Err(std::move(r).unwrap_err());
                ctx.set_value<bool>(hash, r.unwrap());
                return CliResult<void>::Ok();
            }
            case ValueTag::Int:
            {
                auto r = Converter<int>::from_string(s);
                if (r.is_err())
                    return CliResult<void>::Err(std::move(r).unwrap_err());
                ctx.set_value<int>(hash, r.unwrap());
                return CliResult<void>::Ok();
            }
            case ValueTag::Double:
            {
                auto r = Converter<double>::from_string(s);
                if (r.is_err())
                    return CliResult<void>::Err(std::move(r).unwrap_err());
                ctx.set_value<double>(hash, r.unwrap());
                return CliResult<void>::Ok();
            }
            case ValueTag::String:
                ctx.set_value<std::string>(hash, std::string(s));
                return CliResult<void>::Ok();
            case ValueTag::Path:
                ctx.set_value<std::filesystem::path>(hash, std::filesystem::path(s));
                return CliResult<void>::Ok();
            }
            return CliResult<void>::Ok();
        }

        /// @brief Consume a long option token (--opt or --opt=value).
        ///
        /// Looks up the option on @p cmd. If it expects a value:
        ///   - --opt=val: extracts value from the same token
        ///   - --opt val : advances @p i to consume the next token as value
        /// If it is a flag (bool): sets true in @p ctx.
        /// @param cmd   Current command whose options are consulted.
        /// @param ctx   Parse context to write into.
        /// @param arg   The raw token (e.g. "--port=8080").
        /// @param i     Current index into @p args; advanced when a separate
        ///              value token is consumed.  The caller must still
        ///              increment @p i after the call to skip past the
        ///              option (and its value if consumed).
        /// @param args  Full argument list.
        /// @return Ok on success, or a CliError (unknown option, missing
        ///         value, type conversion failure).
        CliResult<void> consume_long(
            const BaseCommand &cmd,
            ParseContext &ctx,
            std::string_view arg,
            size_t &i,
            std::span<const std::string_view> args)
        {
            auto parsed = detail::parse_long_option(arg);

            auto *opt = cmd.find_option_by_long(parsed.name);
            if (!opt)
            {
                if (parsed.is_negation)
                {
                    auto *neg = cmd.find_option_by_long(parsed.negated_name);
                    if (neg && neg->is_negatable())
                    {
                        ctx.set_value<bool>(neg->key_hash(), false);
                        return CliResult<void>::Ok();
                    }
                }
                return CliFailure{unknown_option(std::format("--{}", parsed.name))};
            }

            if (opt->has_value())
            {
                if (parsed.has_equals)
                {
                    if (parsed.value.empty())
                        return CliFailure{missing_value(std::format("--{}", parsed.name))};
                    return opt->parse_value(ctx, parsed.value);
                }
                if (i + 1 >= args.size())
                    return CliFailure{missing_value(std::format("--{}", parsed.name))};
                return opt->parse_value(ctx, args[++i]);
            }

            if (opt->is_counting())
            {
                int cur = ctx.get_value<int>(opt->key_hash(), 0);
                ctx.set_value<int>(opt->key_hash(), cur + 1);
            }
            else
            {
                ctx.set_value<bool>(opt->key_hash(), true);
            }
            return CliResult<void>::Ok();
        }

        /// @brief Consume a short option token (-x, -abc, or -p value).
        ///
        /// Iterates over each character in the token:
        ///   - Bool flags are set to true directly.
        ///   - Valued options must appear as a standalone token (-p value);
        ///     grouped forms like -pvalue or -abp value are rejected.
        ///   When a valued option is found, @p i is advanced to consume the
        ///   next token as its value. The caller must still increment @p i
        ///   after the call.
        /// @param cmd   Current command whose options are consulted.
        /// @param ctx   Parse context to write into.
        /// @param arg   The raw token (e.g. "-abc" or "-p").
        /// @param i     Current index into @p args; advanced when a separate
        ///              value token is consumed.
        /// @param args  Full argument list.
        /// @return Ok on success, or a CliError (unknown option, grouped
        ///         valued option, missing value, type conversion failure).
        CliResult<void> consume_short(
            const BaseCommand &cmd,
            ParseContext &ctx,
            std::string_view arg,
            size_t &i,
            std::span<const std::string_view> args)
        {
            for (size_t j = 1; j < arg.size(); j++)
            {
                char c = arg[j];
                auto *opt = cmd.find_option_by_short(c);
                if (!opt)
                    return CliFailure{unknown_option(std::format("-{}", c))};

                if (opt->has_value())
                {
                    if (arg.size() != 2)
                        return CliFailure{CliError(
                            std::format(
                                "-{} requires a value as a separate argument", c))};
                    if (i + 1 >= args.size())
                        return CliFailure{missing_value(std::format("-{}", c))};
                    auto r = opt->parse_value(ctx, args[++i]);
                    if (r.is_err())
                        return r;
                }
                else if (opt->is_counting())
                {
                    int cur = ctx.get_value<int>(opt->key_hash(), 0);
                    ctx.set_value<int>(opt->key_hash(), cur + 1);
                }
                else
                {
                    ctx.set_value<bool>(opt->key_hash(), true);
                }
            }
            return CliResult<void>::Ok();
        }

        /// @brief Complete parsing: apply defaults, env vars, and check required args.
        /// @param cmd  Current command to finalise.
        /// @param ctx  Mutable parse context.
        /// @return Ok with context, or error on missing required option/arg.
        CliResult<ParseContext> finish_parse(BaseCommand *cmd, ParseContext ctx)
        {
            ctx.set_matched_command(cmd);

            auto dr = cmd->apply_defaults(ctx);
            if (dr.is_err())
                return CliResult<ParseContext>::Err(std::move(dr).unwrap_err());

            for (const auto &opt_ptr : cmd->options())
            {
                if (!ctx.has_value(opt_ptr->key_hash()) && !opt_ptr->env_var().empty())
                {
                    const char *env_val = std::getenv(opt_ptr->env_var().c_str());
                    if (env_val)
                    {
                        auto r = opt_ptr->parse_value(ctx, env_val);
                        if (r.is_err())
                            return CliResult<ParseContext>::Err(
                                std::move(r).unwrap_err());
                    }
                }

                if (opt_ptr->is_required() && !ctx.has_value(opt_ptr->key_hash()))
                    return CliResult<ParseContext>::Err(
                        missing_required_option(opt_ptr->long_name()));
            }

            if (auto *leaf = cmd->as_leaf())
            {
                for (const auto &arg : leaf->args())
                {
                    if (arg.m_required && !ctx.has_value(arg.m_key_hash))
                        return CliResult<ParseContext>::Err(
                            missing_required_arg(arg.m_name));
                }
            }

            return CliResult<ParseContext>::Ok(std::move(ctx));
        }

        /// @brief Try to find a subcommand matching @p name on @p cmd.
        ///        Checks exact match first, then fuzzy if @p max_fuzzy > 0.
        /// @return Match result or nullptr.
        ///         On disabled exact match, *out_disabled is set to true.
        BaseCommand *find_subcommand_match(
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
                auto fuzzy = fuzzy_find_subcommands(
                    cmd, name, max_fuzzy_distance, Visibility::Both);
                if (fuzzy.size() == 1)
                    return fuzzy[0].command;
            }

            return nullptr;
        }

    }  // namespace

    /// @brief Parse args against a command tree.
    /// @param root               Root of the command tree.
    /// @param args               Tokenised CLI arguments.
    /// @param max_fuzzy_distance  0 = exact only, >0 = also try fuzzy (Levenshtein).
    CliResult<ParseContext> parse_command(
        BaseCommand &root, std::span<const std::string_view> args, int max_fuzzy_distance)
    {
        BaseCommand *cmd = &root;
        ParseContext ctx;
        size_t arg_pos = 0;
        bool double_dash = false;

        for (size_t i = 0; i < args.size(); i++)
        {
            auto a = args[i];

            if (!double_dash && a == "--")
            {
                double_dash = true;
                continue;
            }

            if (!double_dash && a.size() > 1 && a[0] == '-')
            {
                CliResult<void> r = (a[1] == '-') ? consume_long(*cmd, ctx, a, i, args)
                                                  : consume_short(*cmd, ctx, a, i, args);
                if (r.is_err())
                    return CliResult<ParseContext>::Err(std::move(r).unwrap_err());
                continue;
            }

            if (!double_dash && cmd->is_branch())
            {
                auto *branch = cmd->as_branch();
                bool disabled = false;
                auto *sub =
                    find_subcommand_match(*branch, a, max_fuzzy_distance, disabled);

                if (disabled)
                    return CliResult<ParseContext>::Err(command_disabled(a));

                if (sub)
                {
                    cmd = sub;
                    ParseContext child_ctx;
                    child_ctx.set_parent(std::make_shared<ParseContext>(std::move(ctx)));
                    ctx = std::move(child_ctx);
                    arg_pos = 0;
                    continue;
                }
            }

            if (auto *leaf = cmd->as_leaf(); leaf && arg_pos < leaf->args().size())
            {
                auto &arg = leaf->args()[arg_pos];
                auto r = apply_arg_value(ctx, arg.m_key_hash, arg.m_value_tag, a);
                if (r.is_err())
                    return CliResult<ParseContext>::Err(std::move(r).unwrap_err());
            }
            else
            {
                switch (cmd->extra_args_policy())
                {
                case ExtraArgsPolicy::Error:
                    return CliFailure{parse_error(a, static_cast<int>(i))};
                case ExtraArgsPolicy::Store:
                    ctx.add_extra_arg(std::string(a));
                    break;
                default:
                    break;
                }
            }
            arg_pos++;
        }

        return finish_parse(cmd, std::move(ctx));
    }

}  // namespace pjh::cli
