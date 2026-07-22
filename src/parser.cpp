#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <format>
#include <memory>
#include <mutex>
#include <pjh_cli/command/base_command.hpp>
#include <pjh_cli/command/branch_command.hpp>
#include <pjh_cli/command/leaf_command.hpp>
#include <pjh_cli/core/converter.hpp>
#include <pjh_cli/core/error.hpp>
#include <pjh_cli/core/type.hpp>
#include <pjh_cli/detail/tokenizer.hpp>
#include <pjh_cli/format/help_formatter.hpp>
#include <pjh_cli/format/matcher.hpp>
#include <pjh_cli/parse/parse_context.hpp>
#include <pjh_cli/parse/parser.hpp>
#include <span>
#include <string>
#include <string_view>
#include <utility>

namespace
{
    /// @brief Check if a raw token looks like an option flag (--opt or -v).
    ///        Negative numbers like -5 or -3.14 are NOT detected as flags.
    bool is_option_flag(std::string_view s) noexcept
    {
        return s.size() > 1 && s[0] == '-' &&
               !std::isdigit(static_cast<unsigned char>(s[1])) && s[1] != '.';
    }
}

namespace pjh::cli
{

    CliResult<void> Parser::apply_arg_value(
        ParseContext &ctx, size_t hash, ValueTag tag, std::string_view s)
    {
        switch (tag)
        {
        case ValueTag::Bool:
            return convert_and_set<bool>(ctx, hash, s);
        case ValueTag::Int:
            return convert_and_set<int>(ctx, hash, s);
        case ValueTag::Double:
            return convert_and_set<double>(ctx, hash, s);
        case ValueTag::String:
            return convert_and_set<std::string>(ctx, hash, s);
        case ValueTag::Path:
            return convert_and_set<std::filesystem::path>(ctx, hash, s);
        }
        return CliResult<void>::Ok();
    }

    void Parser::apply_flag(const OptionDef *opt, ParseContext &ctx)
    {
        if (opt->is_counting())
        {
            int cur = ctx.get_value<int>(opt->key_hash(), 0);
            ctx.set_value<int>(opt->key_hash(), cur + 1);
        }
        else
        {
            ctx.set_value<bool>(opt->key_hash(), true);
        }
    }

    CliResult<void> Parser::consume_long(
        const BaseCommand &cmd,
        ParseContext &ctx,
        std::string_view arg,
        size_t &i,
        std::span<const std::string_view> args)
    {
        auto parsed = detail::Tokenizer::parse_long_option(arg);

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
            return CliFailure{
                ErrorFactory::unknown_option(std::format("--{}", parsed.name))};
        }

        if (opt->has_value())
        {
            if (parsed.has_equals)
            {
                if (parsed.value.empty())
                    return CliFailure{
                        ErrorFactory::missing_value(std::format("--{}", parsed.name))};
                return opt->parse_value(ctx, parsed.value);
            }
            if (i + 1 >= args.size())
                return CliFailure{
                    ErrorFactory::missing_value(std::format("--{}", parsed.name))};

            auto next = args[i + 1];
            if (is_option_flag(next))
                return CliFailure{
                    ErrorFactory::missing_value(std::format("--{}", parsed.name))};

            auto r = opt->parse_value(ctx, args[++i]);
            if (r.is_err())
                return r;

            while (opt->is_repeatable() && i + 1 < args.size())
            {
                next = args[i + 1];
                if (is_option_flag(next))
                    break;
                r = opt->parse_value(ctx, args[++i]);
                if (r.is_err())
                    return r;
            }
            return CliResult<void>::Ok();
        }

        apply_flag(opt, ctx);
        return CliResult<void>::Ok();
    }

    CliResult<void> Parser::consume_short(
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
                return CliFailure{ErrorFactory::unknown_option(std::format("-{}", c))};

            if (opt->has_value())
            {
                // Compact form: -p8080 → value is everything after 'p'
                if (j + 1 < arg.size())
                {
                    auto r = opt->parse_value(ctx, arg.substr(j + 1));
                    if (r.is_err())
                        return r;

                    while (opt->is_repeatable() && i + 1 < args.size())
                    {
                        auto nxt = args[i + 1];
                        if (!nxt.empty() && nxt[0] == '-')
                            break;
                        r = opt->parse_value(ctx, args[++i]);
                        if (r.is_err())
                            return r;
                    }
                    break;
                }

                // Separated form: -p 8080 or -vp 8080
                if (i + 1 >= args.size())
                    return CliFailure{ErrorFactory::missing_value(std::format("-{}", c))};

                auto next = args[i + 1];
                if (is_option_flag(next))
                    return CliFailure{ErrorFactory::missing_value(std::format("-{}", c))};

                auto r = opt->parse_value(ctx, args[++i]);
                if (r.is_err())
                    return r;

                while (opt->is_repeatable() && i + 1 < args.size())
                {
                    next = args[i + 1];
                    if (is_option_flag(next))
                        break;
                    r = opt->parse_value(ctx, args[++i]);
                    if (r.is_err())
                        return r;
                }
            }
            else
            {
                apply_flag(opt, ctx);
            }
        }
        return CliResult<void>::Ok();
    }

    CliResult<ParseContext> Parser::finish_parse(BaseCommand *cmd, ParseContext ctx)
    {
        ctx.set_matched_command(cmd);

        std::vector<BaseCommand *> chain;
        for (auto *c = cmd; c; c = c->parent()) chain.push_back(c);
        std::reverse(chain.begin(), chain.end());

        for (auto *c : chain)
        {
            auto dr = c->apply_defaults(ctx);
            if (dr.is_err())
                return CliResult<ParseContext>::Err(std::move(dr).unwrap_err());

            for (const auto &opt_ptr : c->options())
            {
                if (!ctx.has_value(opt_ptr->key_hash()) && !opt_ptr->env_var().empty())
                {
                    static std::mutex env_mutex;
                    std::string env_val;
                    {
                        std::lock_guard<std::mutex> lock(env_mutex);
                        const char *raw = std::getenv(opt_ptr->env_var().c_str());
                        if (raw)
                            env_val = raw;
                    }
                    if (!env_val.empty())
                    {
                        auto r = opt_ptr->parse_value(ctx, env_val);
                        if (r.is_err())
                            return CliResult<ParseContext>::Err(
                                std::move(r).unwrap_err());
                    }
                }

                if (opt_ptr->is_required() && !ctx.has_value(opt_ptr->key_hash()))
                    return CliResult<ParseContext>::Err(
                        ErrorFactory::missing_required_option(opt_ptr->long_name()));
            }
        }

        if (auto *leaf = cmd->as_leaf())
        {
            for (const auto &arg : leaf->args())
            {
                if (arg.m_required && !ctx.has_value(arg.m_key_hash))
                    return CliResult<ParseContext>::Err(
                        ErrorFactory::missing_required_arg(arg.m_name));
            }
        }

        return CliResult<ParseContext>::Ok(std::move(ctx));
    }

    BaseCommand *Parser::find_subcommand_match(
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
    pjh::result::Option<ParseContext> Parser::try_handle_help(
        BaseCommand *cmd, ParseContext &&ctx, std::string_view a, bool double_dash)
    {
        if (double_dash || (a != "--help" && a != "-h"))
            return pjh::result::Option<ParseContext>::None();

        ctx.set_help_text(HelpFormatter::format_help(*cmd, cmd->name()));
        ctx.set_matched_command(cmd);
        return pjh::result::Option<ParseContext>::Some(std::move(ctx));
    }

    pjh::result::Option<ParseContext> Parser::try_handle_version(
        BaseCommand &root, ParseContext &&ctx, std::string_view a, bool double_dash)
    {
        if (double_dash || a != "--version")
            return pjh::result::Option<ParseContext>::None();

        auto &ver = root.version();
        ctx.set_version_text(std::format("{} version {}\n", root.name(), ver));
        ctx.set_matched_command(&root);
        return pjh::result::Option<ParseContext>::Some(std::move(ctx));
    }

    CliResult<Parser::SubcommandResult> Parser::try_descend_subcommand(
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
        child_ctx.set_parent(std::make_shared<ParseContext>(std::move(ctx)));
        SubcommandResult r;
        r.matched = true;
        r.cmd = sub;
        r.ctx = std::move(child_ctx);
        return CliResult<SubcommandResult>::Ok(std::move(r));
    }

    CliResult<void> Parser::handle_extra_arg(
        const BaseCommand *cmd, ParseContext &ctx, std::string_view a, size_t pos)
    {
        switch (cmd->extra_args_policy())
        {
        case ExtraArgsPolicy::Error:
            return CliFailure{ErrorFactory::parse_error(a, static_cast<int>(pos))};
        case ExtraArgsPolicy::Store:
            ctx.add_extra_arg(std::string(a));
            break;
        default:
            break;
        }
        return CliResult<void>::Ok();
    }

    CliResult<ParseContext> Parser::parse_command(
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

            auto help = try_handle_help(cmd, std::move(ctx), a, double_dash);
            if (help.is_some())
                return CliResult<ParseContext>::Ok(std::move(help).unwrap());

            {
                auto ver = try_handle_version(root, std::move(ctx), a, double_dash);
                if (ver.is_some())
                    return CliResult<ParseContext>::Ok(std::move(ver).unwrap());
            }

            if (!double_dash && a.size() > 1 && a[0] == '-')
            {
                CliResult<void> r = (a[1] == '-') ? consume_long(*cmd, ctx, a, i, args)
                                                  : consume_short(*cmd, ctx, a, i, args);
                if (r.is_err())
                    return CliResult<ParseContext>::Err(std::move(r).unwrap_err());
                continue;
            }

            auto sr =
                try_descend_subcommand(cmd, ctx, a, max_fuzzy_distance, double_dash);
            if (sr.is_err())
                return CliResult<ParseContext>::Err(std::move(sr).unwrap_err());
            if (sr.unwrap().matched)
            {
                auto &r = sr.unwrap();
                cmd = r.cmd;
                ctx = std::move(r.ctx);
                arg_pos = 0;
                continue;
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
                auto r = handle_extra_arg(cmd, ctx, a, i);
                if (r.is_err())
                    return CliResult<ParseContext>::Err(std::move(r).unwrap_err());
            }
            arg_pos++;
        }

        return finish_parse(cmd, std::move(ctx));
    }

}  // namespace pjh::cli
