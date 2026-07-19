#include <pjh_cli/detail/apply_value.hpp>
#include <pjh_cli/detail/string_utils.hpp>
#include <pjh_cli/matcher.hpp>
#include <pjh_cli/parser.hpp>

namespace pjh::cli
{
    namespace
    {
        CliResult<void> consume_long(
            const Command &cmd,
            ParseContext &ctx,
            std::string_view arg,
            size_t &i,
            std::span<const std::string_view> args)
        {
            auto [name, value, has_eq] = detail::split_name_value(arg.substr(2));

            auto *opt = cmd.find_option_by_long(name);
            if (!opt)
                return CliFailure{unknown_option(std::format("--{}", name))};

            if (opt->has_value())
            {
                if (has_eq)
                {
                    if (value.empty())
                        return CliFailure{missing_value(std::format("--{}", name))};
                    return detail::apply_value(
                        ctx, opt->key_hash(), opt->value_tag(), value);
                }
                if (++i >= args.size())
                    return CliFailure{missing_value(std::format("--{}", name))};
                return detail::apply_value(
                    ctx, opt->key_hash(), opt->value_tag(), args[i]);
            }

            ctx.set_value<bool>(opt->key_hash(), true);
            return CliResult<void>::Ok();
        }

        CliResult<void> consume_short(
            const Command &cmd,
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
                    if (++i >= args.size())
                        return CliFailure{missing_value(std::format("-{}", c))};
                    auto r = detail::apply_value(
                        ctx, opt->key_hash(), opt->value_tag(), args[i]);
                    if (r.is_err())
                        return r;
                }
                else
                {
                    ctx.set_value<bool>(opt->key_hash(), true);
                }
            }
            return CliResult<void>::Ok();
        }

        CliResult<void> process_remaining(
            const Command *cmd,
            ParseContext &ctx,
            size_t start,
            std::span<const std::string_view> args)
        {
            size_t arg_pos = 0;
            bool double_dash = false;

            for (size_t i = start; i < args.size(); i++)
            {
                auto a = args[i];

                if (!double_dash && a == "--")
                {
                    double_dash = true;
                    continue;
                }

                if (!double_dash && a.size() > 1 && a[0] == '-')
                {
                    CliResult<void> r = (a[1] == '-')
                                            ? consume_long(*cmd, ctx, a, i, args)
                                            : consume_short(*cmd, ctx, a, i, args);
                    if (r.is_err())
                        return r;
                }
                else
                {
                    if (arg_pos < cmd->args().size())
                    {
                        auto &arg = cmd->args()[arg_pos];
                        auto r =
                            detail::apply_value(ctx, arg.m_key_hash, arg.m_value_tag, a);
                        if (r.is_err())
                            return r;
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
            }

            return CliResult<void>::Ok();
        }

        CliResult<ParseContext> finish_parse(
            const Command *cmd, ParseContext ctx, std::string matched_path)
        {
            ctx.set_matched_command(cmd);
            ctx.set_matched_path(std::move(matched_path));

            auto dr = cmd->apply_defaults(ctx);
            if (dr.is_err())
                return CliResult<ParseContext>::Err(std::move(dr).unwrap_err());

            for (const auto &opt : cmd->options())
            {
                if (opt.is_required() && !ctx.has_value(opt.key_hash()))
                {
                    return CliResult<ParseContext>::Err(
                        missing_required_option(opt.long_name()));
                }
            }

            for (const auto &arg : cmd->args())
            {
                if (arg.m_required && !ctx.has_value(arg.m_key_hash))
                {
                    return CliResult<ParseContext>::Err(missing_required_arg(arg.m_name));
                }
            }

            return CliResult<ParseContext>::Ok(std::move(ctx));
        }

    }  // namespace

    CliResult<ParseContext> parse_command(
        const Command &root,
        std::span<const std::string_view> args,
        int max_fuzzy_distance)
    {
        const Command *cmd = &root;
        size_t i = 0;
        while (i < args.size())
        {
            auto *sub = cmd->find_subcommand(args[i]);
            if (sub)
            {
                if (sub->is_enabled())
                {
                    cmd = sub;
                    i++;
                    continue;
                }
                return CliResult<ParseContext>::Err(command_disabled(args[i]));
            }

            if (max_fuzzy_distance > 0)
            {
                auto fuzzy = fuzzy_find_subcommands(
                    *cmd, args[i], max_fuzzy_distance, Visibility::Both);
                if (fuzzy.size() == 1)
                {
                    cmd = fuzzy[0].command;
                    i++;
                    continue;
                }
            }

            break;
        }

        ParseContext ctx;
        auto r = process_remaining(cmd, ctx, i, args);
        if (r.is_err())
            return CliResult<ParseContext>::Err(std::move(r).unwrap_err());

        return finish_parse(cmd, std::move(ctx), cmd->name());
    }

}  // namespace pjh::cli
