#include <algorithm>
#include <format>
#include <pjh_cli/command/base_command.hpp>
#include <pjh_cli/command/branch_command.hpp>
#include <pjh_cli/command/leaf_command.hpp>
#include <pjh_cli/detail/command_utils.hpp>
#include <pjh_cli/detail/string_utils.hpp>
#include <pjh_cli/help_formatter.hpp>
#include <pjh_cli/info.hpp>
#include <sstream>

namespace pjh::cli
{

    void HelpFormatter::append_help_line(
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

    std::string HelpFormatter::option_label(const OptionInfo &opt, std::string_view sep)
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

    HelpInfo HelpFormatter::collect_help(
        const BaseCommand &cmd, std::string_view program_name, Visibility visibility)
    {
        HelpInfo info;
        info.program_name = program_name;
        info.description = cmd.description();

        info.options.reserve(cmd.options().size());
        for (const auto &opt_ptr : cmd.options()) info.options.emplace_back(*opt_ptr);

        if (auto *leaf = cmd.as_leaf())
        {
            info.args.reserve(leaf->args().size());
            for (const auto &arg : leaf->args()) info.args.emplace_back(arg);
        }

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

    std::string HelpFormatter::format_help(const HelpInfo &info)
    {
        std::ostringstream os;

        auto format_section = [&os](
                                  const std::string &header, const auto &items,
                                  size_t width_limit, auto left_of, auto right_of)
        {
            if (items.empty())
                return;
            os << header << ":\n";
            size_t max_left = 0;
            for (const auto &item : items)
                max_left = std::max(max_left, left_of(item).size());
            size_t left_width = std::min(max_left, width_limit);
            for (const auto &item : items)
            {
                std::string right(right_of(item));
                append_help_line(os, left_of(item), right, left_width);
            }
            os << "\n";
        };

        // Usage
        os << "Usage: ";
        if (!info.program_name.empty())
            os << info.program_name;
        else
            os << "app";

        for (const auto &opt : info.options)
        {
            auto label = option_label(opt, "|");
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
        format_section(
            "Options", info.options, 32,
            [](const OptionInfo &o) { return option_label(o); },
            [](const OptionInfo &o)
            {
                std::string r(o.description);
                if (o.is_required)
                    r += " (required)";
                if (o.has_default)
                    r += " (default: " + o.default_str + ")";
                return r;
            });

        // Arguments section
        format_section(
            "Arguments", info.args, 28,
            [](const ArgInfo &a) { return std::string(a.name); },
            [](const ArgInfo &a)
            {
                std::string r(a.description);
                if (a.is_required)
                    r += " (required)";
                return r;
            });

        // Subcommands section
        format_section(
            "Subcommands", info.subcommands, 28,
            [](const SubcommandInfo &s) { return std::string(s.name); },
            [](const SubcommandInfo &s) { return std::string(s.description); });

        return os.str();
    }

    std::string HelpFormatter::format_usage(
        const BaseCommand &cmd, std::string_view program_name)
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

    std::string HelpFormatter::format_help(
        const BaseCommand &cmd, std::string_view program_name)
    {
        return format_help(collect_help(cmd, program_name));
    }

}  // namespace pjh::cli
