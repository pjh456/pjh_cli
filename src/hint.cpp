#include <pjh_cli/command/branch_command.hpp>
#include <pjh_cli/command/leaf_command.hpp>
#include <pjh_cli/detail/tokenizer.hpp>
#include <pjh_cli/format/hint.hpp>
#include <pjh_cli/format/info.hpp>
#include <sstream>
#include <string>
#include <vector>

namespace pjh::cli
{

    namespace
    {
        std::string type_name(ValueTag tag, bool is_counting)
        {
            if (is_counting)
                return "INT";
            switch (tag)
            {
            case ValueTag::Int:
                return "INT";
            case ValueTag::Bool:
                return "BOOL";
            case ValueTag::String:
                return "STR";
            case ValueTag::Double:
                return "FLOAT";
            case ValueTag::Path:
                return "PATH";
            }
            return "STR";
        }

        std::string option_label(
            ValueTag tag, bool is_counting, std::string_view long_name, char short_name)
        {
            auto label = type_name(tag, is_counting) + ":";
            if (!long_name.empty())
                label += long_name;
            else if (short_name != 0)
                label += short_name;
            return label;
        }
    }

    std::string HintBuilder::option_type_name(const OptionDef &opt)
    {
        return type_name(opt.value_tag(), opt.is_counting());
    }

    // ── Data collection ──

    HintContext HintBuilder::build_context(
        const BaseCommand &root, std::string_view input)
    {
        auto tokens = detail::Tokenizer::tokenize(input);
        const BaseCommand *cmd = &root;
        size_t arg_pos = 0;

        for (const auto &tok : tokens)
        {
            if (tok.size() > 1 && tok[0] == '-')
                continue;

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

        ctx.options.reserve(cmd->options().size());
        for (const auto &opt_ptr : cmd->options()) ctx.options.emplace_back(*opt_ptr);

        if (auto *leaf = cmd->as_leaf())
        {
            auto &args = leaf->args();
            ctx.remaining_args.reserve(args.size());
            for (size_t i = arg_pos; i < args.size(); i++)
                ctx.remaining_args.emplace_back(args[i]);
        }

        return ctx;
    }

    // ── build_hint ──

    HintInfo HintBuilder::build_hint(const HintContext &ctx, HintConfig config)
    {
        HintInfo info;

        auto add_option = [&](const OptionInfo &opt)
        {
            auto label = option_label(
                opt.value_tag, opt.is_counting, opt.long_name, opt.short_name);
            HintToken tok;
            if (opt.is_required)
                tok.display = label;
            else
                tok.display = "[" + label + "]";
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
            tok.display = "<" + std::string(arg.name) + ">";
            info.tokens.push_back(std::move(tok));
        }

        return info;
    }

    // ── format(HintInfo) ──

    std::string HintBuilder::format(const HintInfo &info)
    {
        if (info.tokens.empty())
            return {};

        std::ostringstream os;
        for (const auto &tok : info.tokens)
            os << tok.display << " ";
        auto result = os.str();
        result.pop_back();
        return result;
    }

    // ── format(HintContext) — adapter ──

    std::string HintBuilder::format(const HintContext &ctx)
    {
        return format(build_hint(ctx));
    }

    // ── format(BaseCommand, input, config) — adapter ──

    std::string HintBuilder::format(
        const BaseCommand &root, std::string_view input, HintConfig config)
    {
        auto ctx = build_context(root, input);
        return format(build_hint(ctx, config));
    }

}  // namespace pjh::cli
