#include <pjh_cli/app.hpp>
#include <pjh_cli/matcher.hpp>

#include <cctype>
#include <vector>

namespace pjh::cli
{
    App::App(
        std::string name,
        std::string version,
        std::string description)
        : Command(
              std::move(name),
              std::move(description)),
          m_version(std::move(version))
    {
    }

    namespace
    {
        ParseResult<void>
        consume_long(
            const Command &cmd,
            ParseContext &ctx,
            std::string_view arg,
            size_t &i,
            const std::vector<std::string_view> &args)
        {
            auto name = arg.substr(2);
            std::string_view value;
            bool has_eq = false;

            auto eq = name.find('=');
            if (eq != std::string_view::npos)
            {
                value = name.substr(eq + 1);
                name = name.substr(0, eq);
                has_eq = true;
            }

            auto *opt = cmd.find_option_by_long(name);
            if (!opt)
                return ParseFailure{
                    unknown_option(
                        std::string("--") + std::string(name))};

            if (opt->m_has_value)
            {
                if (has_eq)
                    return opt->m_apply(ctx, value);
                if (++i >= args.size())
                    return ParseFailure{
                        missing_value(
                            std::string("--") + std::string(name))};
                return opt->m_apply(ctx, args[i]);
            }

            return opt->m_apply(ctx, "true");
        }

        ParseResult<void>
        consume_short(
            const Command &cmd,
            ParseContext &ctx,
            std::string_view arg,
            size_t &i,
            const std::vector<std::string_view> &args)
        {
            for (size_t j = 1; j < arg.size(); j++)
            {
                char c = arg[j];
                auto *opt = cmd.find_option_by_short(c);
                if (!opt)
                    return ParseFailure{
                        unknown_option(
                            std::string("-") + c)};

                if (opt->m_has_value)
                {
                    if (j + 1 < arg.size())
                    {
                        auto r = opt->m_apply(
                            ctx, arg.substr(j + 1));
                        if (r.is_err())
                            return r;
                        break;
                    }
                    if (++i >= args.size())
                        return ParseFailure{
                            missing_value(
                                std::string("-") + c)};
                    auto r = opt->m_apply(ctx, args[i]);
                    if (r.is_err())
                        return r;
                }
                else
                {
                    auto r = opt->m_apply(ctx, "true");
                    if (r.is_err())
                        return r;
                }
            }
            return ParseResult<void>::Ok();
        }

        ParseResult<void>
        process_remaining(
            const Command *cmd,
            ParseContext &ctx,
            size_t start,
            const std::vector<std::string_view> &args)
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
                    ParseResult<void> r =
                        (a[1] == '-')
                            ? consume_long(*cmd, ctx, a, i, args)
                            : consume_short(*cmd, ctx, a, i, args);
                    if (r.is_err())
                        return r;
                }
                else
                {
                    if (arg_pos < cmd->args().size())
                    {
                        auto r = cmd->args()[arg_pos].m_apply(ctx, a);
                        if (r.is_err())
                            return r;
                    }
                    arg_pos++;
                }
            }

            return ParseResult<void>::Ok();
        }

        ParseResult<ParseContext>
        finish_parse(
            const Command *cmd,
            ParseContext ctx,
            std::string matched_path)
        {
            ctx.set_matched_path(std::move(matched_path));

            // Apply defaults
            auto dr = cmd->apply_defaults(ctx);
            if (dr.is_err())
                return ParseResult<ParseContext>::Err(
                    std::move(dr).unwrap_err());

            // Validate required options
            for (const auto &opt : cmd->options())
            {
                if (opt.m_required &&
                    !ctx.has_value(opt.m_key_hash))
                {
                    return ParseResult<ParseContext>::Err(
                        missing_required_option(
                            opt.m_long_name));
                }
            }

            // Validate required positional args
            for (const auto &arg : cmd->args())
            {
                if (arg.m_required &&
                    !ctx.has_value(arg.m_key_hash))
                {
                    return ParseResult<ParseContext>::Err(
                        missing_required_arg(
                            arg.m_name));
                }
            }

            return ParseResult<ParseContext>::Ok(
                std::move(ctx));
        }

    } // namespace

    ParseResult<ParseContext>
    App::parse(
        int argc,
        char **argv)
    {
        std::vector<std::string_view> args;
        for (int a = 1; a < argc; a++)
            args.emplace_back(argv[a]);

        const Command *cmd = this;
        size_t i = 0;
        while (i < args.size())
        {
            auto *sub = cmd->find_subcommand(args[i]);
            if (sub && sub->is_enabled())
            {
                cmd = sub;
                i++;
            }
            else
            {
                break;
            }
        }

        ParseContext ctx;
        auto r = process_remaining(cmd, ctx, i, args);
        if (r.is_err())
            return ParseResult<ParseContext>::Err(
                std::move(r).unwrap_err());

        return finish_parse(cmd, std::move(ctx), cmd->name());
    }

    ParseResult<ParseContext>
    App::parse_fuzzy(
        int argc,
        char **argv)
    {
        std::vector<std::string_view> args;
        for (int a = 1; a < argc; a++)
            args.emplace_back(argv[a]);

        const Command *cmd = this;
        size_t i = 0;
        while (i < args.size())
        {
            auto *sub = cmd->find_subcommand(args[i]);
            if (sub && sub->is_enabled())
            {
                cmd = sub;
                i++;
                continue;
            }

            auto fuzzy = fuzzy_find_subcommands(
                *cmd, args[i], 3, Visibility::Both);

            if (fuzzy.size() == 1)
            {
                cmd = fuzzy[0].command;
                i++;
                continue;
            }

            break;
        }

        ParseContext ctx;
        auto r = process_remaining(cmd, ctx, i, args);
        if (r.is_err())
            return ParseResult<ParseContext>::Err(
                std::move(r).unwrap_err());

        return finish_parse(cmd, std::move(ctx), cmd->name());
    }

} // namespace pjh::cli
