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
    }

    std::string HintBuilder::option_type_name(const OptionDef &opt)
    {
        return type_name(opt.value_tag(), opt.is_counting());
    }

    // ── Data collection ──

    HintContext HintBuilder::build_context(
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

    // ── Basic renderer ──

    namespace
    {
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

        void trim_trailing_space(std::string &s)
        {
            if (!s.empty() && s.back() == ' ')
                s.pop_back();
        }
    }

    std::string HintBuilder::format(const HintContext &ctx)
    {
        std::ostringstream os;

        for (const auto &opt : ctx.options)
        {
            auto label = option_label(
                opt.value_tag, opt.is_counting, opt.long_name, opt.short_name);

            if (opt.is_required)
                os << label << " ";
            else
                os << "[" << label << "] ";
        }

        for (const auto &arg : ctx.remaining_args) os << "<" << arg.name << "> ";

        auto result = os.str();
        trim_trailing_space(result);
        return result;
    }

    // ── Backward-compatible wrapper ──

    std::string HintBuilder::format(
        const BaseCommand &root, std::string_view input, HintConfig config)
    {
        auto ctx = build_context(root, input);

        if (config.option_mode == HintOptionMode::All)
            return format(ctx);

        std::ostringstream os;

        if (config.option_mode != HintOptionMode::None)
        {
            for (const auto &opt_ptr : ctx.reached_command->options())
            {
                bool show =
                    (config.option_mode == HintOptionMode::Required &&
                     opt_ptr->is_required());
                if (!show)
                    continue;

                auto label = option_label(
                    opt_ptr->value_tag(), opt_ptr->is_counting(), opt_ptr->long_name(),
                    opt_ptr->short_name());

                if (opt_ptr->is_required())
                    os << label << " ";
                else
                    os << "[" << label << "] ";
            }
        }

        for (const auto &arg : ctx.remaining_args) os << "<" << arg.name << "> ";

        auto result = os.str();
        trim_trailing_space(result);
        return result;
    }

}  // namespace pjh::cli
