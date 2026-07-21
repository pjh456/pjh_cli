#include <pjh_cli/command/branch_command.hpp>
#include <pjh_cli/command/leaf_command.hpp>
#include <pjh_cli/detail/tokenizer.hpp>
#include <pjh_cli/hint.hpp>
#include <pjh_cli/info.hpp>
#include <sstream>
#include <string>
#include <vector>

namespace pjh::cli
{

    std::string option_type_name(const OptionDef &opt)
    {
        if (opt.is_counting())
            return "INT";
        switch (opt.value_tag())
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

    // ── Data collection ──

    HintContext build_hint_context(
        const BaseCommand &root, std::string_view input)
    {
        auto tokens = detail::tokenize_input(input);
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
        for (const auto &opt_ptr : cmd->options())
            ctx.options.emplace_back(*opt_ptr);

        if (auto *leaf = cmd->as_leaf())
        {
            auto &args = leaf->args();
            ctx.remaining_args.reserve(args.size());
            for (size_t i = arg_pos; i < args.size(); i++)
                ctx.remaining_args.emplace_back(args[i]);
        }

        return ctx;
    }

    // ── Basic renderer ──

    namespace
    {
        std::string type_name_for_tag(ValueTag tag, bool is_counting)
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
    }  // namespace

    std::string format_hint(const HintContext &ctx)
    {
        std::ostringstream os;

        for (const auto &opt : ctx.options)
        {
            std::string label =
                type_name_for_tag(opt.value_tag, opt.is_counting) + ":";
            if (!opt.long_name.empty())
                label += opt.long_name;
            else if (opt.short_name != 0)
                label += opt.short_name;

            if (opt.is_required)
                os << label << " ";
            else
                os << "[" << label << "] ";
        }

        for (const auto &arg : ctx.remaining_args)
            os << "<" << arg.name << "> ";

        auto result = os.str();
        if (!result.empty() && result.back() == ' ')
            result.pop_back();
        return result;
    }

    // ── Backward-compatible wrapper ──

    std::string format_hint(
        const BaseCommand &root, std::string_view input, HintConfig config)
    {
        auto ctx = build_hint_context(root, input);

        if (config.option_mode == HintOptionMode::All)
            return format_hint(ctx);

        std::ostringstream os;

        if (config.option_mode != HintOptionMode::None)
        {
            for (const auto &opt_ptr : ctx.reached_command->options())
            {
                bool show = (config.option_mode == HintOptionMode::Required &&
                             opt_ptr->is_required());
                if (!show)
                    continue;

                std::string label = option_type_name(*opt_ptr) + ":";
                if (!opt_ptr->long_name().empty())
                    label += opt_ptr->long_name();
                else if (opt_ptr->short_name() != 0)
                    label += opt_ptr->short_name();

                if (opt_ptr->is_required())
                    os << label << " ";
                else
                    os << "[" << label << "] ";
            }
        }

        for (const auto &arg : ctx.remaining_args)
            os << "<" << arg.name << "> ";

        auto result = os.str();
        if (!result.empty() && result.back() == ' ')
            result.pop_back();
        return result;
    }

}  // namespace pjh::cli
