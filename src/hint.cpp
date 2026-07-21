#include <pjh_cli/command/branch_command.hpp>
#include <pjh_cli/command/leaf_command.hpp>
#include <pjh_cli/detail/tokenizer.hpp>
#include <pjh_cli/hint.hpp>
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

    std::string format_hint(
        const BaseCommand &root, std::string_view input, HintConfig config)
    {
        auto tokens = detail::tokenize_input(input);
        const BaseCommand *cmd = &root;
        size_t arg_pos = 0;

        for (const auto &tok : tokens)
        {
            // Skip consumed option tokens
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
                // Not a subcommand name — stop walking
                break;
            }

            // Leaf: treat as consumed positional arg
            arg_pos++;
        }

        std::ostringstream os;

        // Options
        if (config.option_mode != HintOptionMode::None)
        {
            for (const auto &opt_ptr : cmd->options())
            {
                bool show = (config.option_mode == HintOptionMode::All) ||
                            (config.option_mode == HintOptionMode::Required &&
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

        // Positional args (remaining)
        if (auto *leaf = cmd->as_leaf())
        {
            for (size_t i = arg_pos; i < leaf->args().size(); i++)
            {
                os << "<" << leaf->args()[i].m_name << "> ";
            }
        }

        // Trim trailing space
        auto result = os.str();
        if (!result.empty() && result.back() == ' ')
            result.pop_back();
        return result;
    }

}  // namespace pjh::cli
