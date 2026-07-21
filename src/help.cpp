#include <algorithm>
#include <pjh_cli/command/base_command.hpp>
#include <pjh_cli/command/branch_command.hpp>
#include <pjh_cli/command/leaf_command.hpp>
#include <pjh_cli/detail/command_utils.hpp>
#include <pjh_cli/detail/string_utils.hpp>
#include <pjh_cli/info.hpp>
#include <pjh_cli/matcher.hpp>
#include <sstream>

namespace pjh::cli
{
    namespace
    {
        void append_help_line(
            std::ostringstream &os,
            const std::string &left,
            const std::string &right,
            size_t left_width)
        {
            os << "  " << left;
            if (left.size() < left_width)
                os << std::string(left_width - left.size(), ' ');
            os << "  " << right << "\n";
        }

        std::string option_left_label(const OptionInfo &opt, std::string_view sep = ", ")
        {
            std::string left;
            if (opt.short_name != 0)
                left = std::format("-{}", opt.short_name);
            if (opt.short_name != 0 && !opt.long_name.empty())
                left += sep;
            if (!opt.long_name.empty())
                left += std::format("--{}", opt.long_name);
            if (opt.has_value)
            {
                auto label = opt.long_name.empty() ? std::string(1, opt.short_name)
                                                   : std::string(opt.long_name);
                left += " " + detail::to_upper_copy(label);
            }
            return left;
        }
    }  // namespace

    // ── Data collection ──

    HelpInfo collect_help(
        const BaseCommand &cmd, std::string_view program_name, Visibility visibility)
    {
        HelpInfo info;
        info.program_name = program_name;
        info.description = cmd.description();

        // Options
        info.options.reserve(cmd.options().size());
        for (const auto &opt_ptr : cmd.options()) info.options.emplace_back(*opt_ptr);

        // Positional args (leaf only)
        if (auto *leaf = cmd.as_leaf())
        {
            info.args.reserve(leaf->args().size());
            for (const auto &arg : leaf->args()) info.args.emplace_back(arg);
        }

        // Subcommands (branch only)
        if (auto *branch = cmd.as_branch())
        {
            for (const auto &sub_ptr : branch->subcommands())
            {
                if (!detail::is_visible_and_enabled(*sub_ptr, visibility))
                    continue;
                info.subcommands.push_back({sub_ptr->name(), sub_ptr->description()});
            }
        }

        return info;
    }

    // ── Render from structured data ──

    std::string format_help(const HelpInfo &info)
    {
        std::ostringstream os;

        // Usage
        os << "Usage: ";
        if (!info.program_name.empty())
            os << info.program_name;
        else
            os << "app";

        for (const auto &opt : info.options)
        {
            auto label = option_left_label(opt, "|");
            if (opt.is_required)
                os << " " << label;
            else
                os << " [" << label << "]";
        }
        for (const auto &arg : info.args)
        {
            if (arg.is_required)
                os << " <" << arg.name << ">";
            else
                os << " [" << arg.name << "]";
        }
        if (!info.subcommands.empty())
            os << " <command>";
        os << "\n\n";

        // Description
        if (!info.description.empty())
            os << info.description << "\n\n";

        // Options section
        if (!info.options.empty())
        {
            os << "Options:\n";
            size_t max_left = 0;
            for (const auto &opt : info.options)
                max_left = std::max(max_left, option_left_label(opt).size());
            size_t left_width = std::min(max_left, size_t(32));

            for (const auto &opt : info.options)
            {
                std::string right(opt.description);
                if (opt.is_required)
                    right += " (required)";
                if (opt.has_default)
                    right += " (default: " + opt.default_str + ")";
                append_help_line(os, option_left_label(opt), right, left_width);
            }
            os << "\n";
        }

        // Positional args section
        if (!info.args.empty())
        {
            os << "Arguments:\n";
            size_t max_left = 0;
            for (const auto &arg : info.args)
                max_left = std::max(max_left, arg.name.size());
            size_t left_width = std::min(max_left, size_t(28));

            for (const auto &arg : info.args)
            {
                std::string right(arg.description);
                if (arg.is_required)
                    right += " (required)";
                append_help_line(os, std::string(arg.name), right, left_width);
            }
            os << "\n";
        }

        // Subcommands section
        if (!info.subcommands.empty())
        {
            os << "Subcommands:\n";
            size_t max_left = 0;
            for (const auto &sub : info.subcommands)
                max_left = std::max(max_left, sub.name.size());
            size_t left_width = std::min(max_left, size_t(28));

            for (const auto &sub : info.subcommands)
                append_help_line(
                    os, std::string(sub.name), std::string(sub.description), left_width);
        }

        return os.str();
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

        for (const auto &opt_ptr : cmd.options())
        {
            auto label = detail::option_left_label(*opt_ptr, "|");
            if (opt_ptr->is_required())
                os << " " << label;
            else
                os << " [" << label << "]";
        }

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

    // ── Backward-compatible wrapper ──

    std::string format_help(const BaseCommand &cmd, std::string_view program_name)
    {
        return format_help(collect_help(cmd, program_name));
    }

}  // namespace pjh::cli
