#include <algorithm>
#include <pjh_cli/command/base_command.hpp>
#include <pjh_cli/command/branch_command.hpp>
#include <pjh_cli/command/leaf_command.hpp>
#include <pjh_cli/detail/command_utils.hpp>
#include <pjh_cli/matcher.hpp>
#include <sstream>

namespace pjh::cli
{

    static void append_help_line(
        std::ostringstream &os, std::string left, std::string right, size_t left_width)
    {
        os << "  " << left;
        if (left.size() < left_width)
            os << std::string(left_width - left.size(), ' ');
        os << "  " << right << "\n";
    }

    std::string format_usage(const BaseCommand &cmd, std::string_view program_name)
    {
        std::ostringstream os;
        os << "Usage: ";

        if (!program_name.empty())
            os << program_name;
        else if (!cmd.name().empty())
            os << cmd.name();
        else
            os << "app";

        // Options: required without brackets, optional with brackets
        for (const auto &opt_ptr : cmd.options())
        {
            auto label = detail::option_left_label(*opt_ptr, "|");
            if (opt_ptr->is_required())
                os << " " << label;
            else
                os << " [" << label << "]";
        }

        // Positional args (leaf only)
        if (auto *leaf = cmd.as_leaf())
        {
            for (const auto &arg : leaf->args())
            {
                if (arg.m_required)
                    os << " <" << arg.m_name << ">";
                else
                    os << " [" << arg.m_name << "]";
            }
        }

        // Subcommands (branch only)
        if (!cmd.is_leaf())
        {
            auto &branch = static_cast<const BranchCommand &>(cmd);
            auto &subcommands = branch.subcommands();
            bool has_visible = std::any_of(
                subcommands.begin(), subcommands.end(), [](const auto &sub_ptr)
                { return detail::is_visible_and_enabled(*sub_ptr, Visibility::Both); });
            if (has_visible)
                os << " <command>";
        }

        return os.str();
    }

    std::string format_help(const BaseCommand &cmd, std::string_view program_name)
    {
        std::ostringstream os;

        os << format_usage(cmd, program_name) << "\n\n";

        if (!cmd.description().empty())
            os << cmd.description() << "\n\n";

        // Options
        if (!cmd.options().empty())
        {
            os << "Options:\n";

            size_t max_left = 0;
            for (const auto &opt_ptr : cmd.options())
                max_left = std::max(max_left, detail::option_left_label(*opt_ptr).size());
            size_t left_width = std::min(max_left, size_t(32));

            for (const auto &opt_ptr : cmd.options())
            {
                std::string right = opt_ptr->description();
                if (opt_ptr->is_required())
                    right += " (required)";
                if (opt_ptr->has_default())
                    right += " (default: " + opt_ptr->default_value_str() + ")";
                append_help_line(
                    os, detail::option_left_label(*opt_ptr), right, left_width);
            }
            os << "\n";
        }

        // Positional args (leaf only)
        if (auto *leaf = cmd.as_leaf(); leaf && !leaf->args().empty())
        {
            os << "Arguments:\n";
            size_t max_left = 0;
            for (const auto &arg : leaf->args())
                max_left = std::max(max_left, arg.m_name.size());
            size_t left_width = std::min(max_left, size_t(28));

            for (const auto &arg : leaf->args())
            {
                std::string right = arg.m_description;
                if (arg.m_required)
                    right += " (required)";
                append_help_line(os, arg.m_name, right, left_width);
            }
            os << "\n";
        }

        // Subcommands (branch only)
        if (!cmd.is_leaf())
        {
            auto &branch = static_cast<const BranchCommand &>(cmd);
            auto &subcommands = branch.subcommands();

            size_t max_left = 0;
            size_t visible_count = 0;
            for (const auto &sub_ptr : subcommands)
            {
                if (!detail::is_visible_and_enabled(*sub_ptr, Visibility::Both))
                    continue;
                visible_count++;
                max_left = std::max(max_left, sub_ptr->name().size());
            }

            if (visible_count > 0)
            {
                os << "Subcommands:\n";
                size_t left_width = std::min(max_left, size_t(28));
                for (const auto &sub_ptr : subcommands)
                {
                    if (!detail::is_visible_and_enabled(*sub_ptr, Visibility::Both))
                        continue;
                    append_help_line(
                        os, sub_ptr->name(), sub_ptr->description(), left_width);
                }
            }
        }

        return os.str();
    }

}  // namespace pjh::cli
