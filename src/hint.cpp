#include <pjh_cli/command/arg_scan.hpp>
#include <pjh_cli/command/branch_command.hpp>
#include <pjh_cli/command/leaf_command.hpp>
#include <pjh_cli/command/matcher.hpp>
#include <pjh_cli/detail/string_utils.hpp>
#include <pjh_cli/detail/tokenizer.hpp>
#include <pjh_cli/format/hint.hpp>
#include <pjh_cli/format/info.hpp>
#include <string>
#include <vector>

namespace pjh::cli
{

    namespace
    {
        std::string_view type_name(ValueTag tag, bool is_counting)
        {
            if (is_counting)
                return "INT";
            static constexpr auto names =
                detail::hint_names(static_cast<detail::BuiltinTypes *>(nullptr));
            auto idx = static_cast<size_t>(tag);
            if (idx < names.size())
                return names[idx];
            return "STR";
        }

        /// @brief Append one option token ([INT:port] / INT:timeout) to @p out.
        void append_option_display(std::string &out, const OptionInfo &opt)
        {
            if (!opt.is_required)
                out += '[';
            out += type_name(opt.value_tag, opt.is_counting);
            out += ':';
            if (!opt.long_name.empty())
                out += opt.long_name;
            else if (opt.short_name != 0)
                out += opt.short_name;
            if (!opt.is_required)
                out += ']';
        }

        /// @brief Exact byte length of one option token in a rendered hint.
        std::size_t option_display_size(const OptionInfo &opt)
        {
            std::size_t n = type_name(opt.value_tag, opt.is_counting).size() + 1;
            if (!opt.long_name.empty())
                n += opt.long_name.size();
            else if (opt.short_name != 0)
                n += 1;
            if (!opt.is_required)
                n += 2;
            return n;
        }

        /// @brief Render the option/arg tokens of @p ctx (per @p config) into
        ///        @p out, space-separated, in display order.
        void append_hint(std::string &out, const HintContext &ctx, HintConfig config)
        {
            bool first = true;
            auto add = [&](const OptionInfo &opt)
            {
                if (!first)
                    out += ' ';
                first = false;
                append_option_display(out, opt);
            };
            switch (config.option_mode)
            {
            case HintOptionMode::All:
                for (const auto &opt : ctx.options) add(opt);
                break;
            case HintOptionMode::Required:
                for (const auto &opt : ctx.options)
                    if (opt.is_required)
                        add(opt);
                break;
            case HintOptionMode::None:
                break;
            }
            for (const auto &arg : ctx.remaining_args)
            {
                if (!first)
                    out += ' ';
                first = false;
                out += '<';
                out += arg.name;
                out += '>';
            }
        }

        /// @brief Exact byte size of append_hint() output for @p ctx.
        std::size_t hint_render_size(const HintContext &ctx, HintConfig config)
        {
            std::size_t total = 0;
            std::size_t count = 0;
            auto add = [&](const OptionInfo &opt)
            {
                total += option_display_size(opt);
                ++count;
            };
            switch (config.option_mode)
            {
            case HintOptionMode::All:
                for (const auto &opt : ctx.options) add(opt);
                break;
            case HintOptionMode::Required:
                for (const auto &opt : ctx.options)
                    if (opt.is_required)
                        add(opt);
                break;
            case HintOptionMode::None:
                break;
            }
            for (const auto &arg : ctx.remaining_args)
            {
                total += arg.name.size() + 2;
                ++count;
            }
            return count == 0 ? 0 : total + count - 1;
        }
    }

    std::string HintBuilder::option_type_name(const OptionDef &opt)
    {
        return std::string(type_name(opt.value_tag(), opt.is_counting()));
    }

    // ── Data collection ──

    HintContext HintBuilder::build_context(
        const BaseCommand &root, std::string_view input)
    {
        auto tokens = detail::Tokenizer::tokenize_views(input);
        const BaseCommand *cmd = &root;
        size_t arg_pos = 0;

        bool after_double_dash = false;

        for (size_t i = 0; i < tokens.tokens.size(); ++i)
        {
            std::string_view tok = tokens.tokens[i];

            if (after_double_dash)
            {
                arg_pos++;
                continue;
            }
            if (tok == "--")
            {
                after_double_dash = true;
                continue;
            }
            if (detail::is_option_flag(tok))
            {
                auto scan = detail::scan_option_token(*cmd, tok);
                if (scan.option && (scan.needs_next_token || scan.compact_value))
                {
                    if (scan.needs_next_token && i + 1 < tokens.tokens.size() &&
                        !detail::is_option_flag(tokens.tokens[i + 1]))
                        ++i;  // the option's separate value token

                    if (scan.option->is_repeatable())
                    {
                        while (i + 1 < tokens.tokens.size() &&
                               !detail::is_option_flag(tokens.tokens[i + 1]) &&
                               !detail::is_subcommand_of(*cmd, tokens.tokens[i + 1]))
                            ++i;
                    }
                }
                continue;
            }

            if (cmd->is_branch())
            {
                auto *branch = cmd->as_branch();
                auto *sub = branch->find_subcommand(tok);
                if (sub)
                {
                    cmd = sub;
                    arg_pos = 0;
                    continue;
                }
                break;
            }

            arg_pos++;
        }

        HintContext ctx;
        ctx.reached_command = cmd;
        ctx.consumed_positional_args = arg_pos;

        auto chain = detail::collect_options_in_chain(*cmd);
        ctx.options.reserve(chain.size());
        for (const auto &entry : chain)
        {
            OptionInfo opt_info(*entry.opt, /*with_default_str=*/false);
            if (entry.long_shadowed)
                opt_info.long_name = {};
            if (entry.short_shadowed)
                opt_info.short_name = 0;
            ctx.options.push_back(std::move(opt_info));
        }

        if (auto *leaf = cmd->as_leaf())
        {
            auto &args = leaf->args();
            if (arg_pos < args.size())
                ctx.remaining_args.reserve(args.size() - arg_pos);
            for (size_t i = arg_pos; i < args.size(); i++)
                ctx.remaining_args.emplace_back(args[i]);
        }

        return ctx;
    }

    // ── build_hint ──

    HintInfo HintBuilder::build_hint(const HintContext &ctx, HintConfig config)
    {
        HintInfo info;
        info.tokens.reserve(ctx.options.size() + ctx.remaining_args.size());

        auto add_option = [&](const OptionInfo &opt)
        {
            HintToken tok;
            append_option_display(tok.display, opt);
            info.tokens.push_back(std::move(tok));
        };

        switch (config.option_mode)
        {
        case HintOptionMode::All:
            for (const auto &opt : ctx.options)
                add_option(opt);
            break;
        case HintOptionMode::Required:
            for (const auto &opt : ctx.options)
                if (opt.is_required)
                    add_option(opt);
            break;
        case HintOptionMode::None:
            break;
        }

        for (const auto &arg : ctx.remaining_args)
        {
            HintToken tok;
            tok.display += '<';
            tok.display += arg.name;
            tok.display += '>';
            info.tokens.push_back(std::move(tok));
        }

        return info;
    }

    // ── format(HintInfo) ──

    std::string HintBuilder::format(const HintInfo &info)
    {
        if (info.tokens.empty())
            return {};

        std::size_t total = 0;
        for (const auto &tok : info.tokens) total += tok.display.size() + 1;
        std::string result;
        result.reserve(total - 1);
        bool first = true;
        for (const auto &tok : info.tokens)
        {
            if (!first)
                result += ' ';
            first = false;
            result += tok.display;
        }
        return result;
    }

    // ── format(HintContext) — adapter ──

    std::string HintBuilder::format(const HintContext &ctx)
    {
        HintConfig config{};
        std::string out;
        out.reserve(hint_render_size(ctx, config));
        append_hint(out, ctx, config);
        return out;
    }

    // ── format(BaseCommand, input, config) — adapter ──

    std::string HintBuilder::format(
        const BaseCommand &root, std::string_view input, HintConfig config)
    {
        auto ctx = build_context(root, input);
        std::string out;
        out.reserve(hint_render_size(ctx, config));
        append_hint(out, ctx, config);
        return out;
    }

}  // namespace pjh::cli
