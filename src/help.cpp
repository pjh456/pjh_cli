#include <algorithm>
#include <format>
#include <pjh_cli/command/base_command.hpp>
#include <pjh_cli/command/branch_command.hpp>
#include <pjh_cli/command/leaf_command.hpp>
#include <pjh_cli/detail/command_utils.hpp>
#include <pjh_cli/detail/string_utils.hpp>
#include <pjh_cli/format/help_formatter.hpp>
#include <pjh_cli/format/info.hpp>
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
            left += " " + detail::StringUtils::to_upper_copy(label);
            if (opt.is_repeatable)
                left += " [...]";
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
                std::vector<std::string> aliases(
                    sub_ptr->aliases().begin(), sub_ptr->aliases().end());
                info.subcommands.push_back(
                    {sub_ptr->name(), sub_ptr->description(), std::move(aliases)});
            }
        }

        return info;
    }

    // ── build_document ──

    HelpDocument HelpFormatter::build_document(const HelpInfo &info)
    {
        HelpDocument doc;
        doc.usage = build_usage(info);
        doc.description = info.description;

        auto make_section = [](std::string heading, auto &&add_lines) -> HelpSection
        {
            HelpSection sec;
            sec.heading = std::move(heading);
            add_lines(sec.lines);
            return sec;
        };

        // Options
        if (!info.options.empty())
        {
            doc.sections.push_back(make_section(
                "Options",
                [&](std::vector<HelpLine> &lines)
                {
                    for (const auto &opt : info.options)
                    {
                        HelpLine line;
                        line.left = option_label(opt);
                        line.right = std::string(opt.description);
                        if (opt.is_required)
                            line.right += " (required)";
                        if (opt.has_default)
                            line.right += " (default: " + opt.default_str + ")";
                        lines.push_back(std::move(line));
                    }
                }));
        }

        // Arguments
        if (!info.args.empty())
        {
            doc.sections.push_back(make_section(
                "Arguments",
                [&](std::vector<HelpLine> &lines)
                {
                    for (const auto &arg : info.args)
                    {
                        HelpLine line;
                        line.left = std::string(arg.name);
                        line.right = std::string(arg.description);
                        if (arg.is_required)
                            line.right += " (required)";
                        lines.push_back(std::move(line));
                    }
                }));
        }

        // Subcommands
        if (!info.subcommands.empty())
        {
            doc.sections.push_back(make_section(
                "Subcommands",
                [&](std::vector<HelpLine> &lines)
                {
                    for (const auto &sub : info.subcommands)
                    {
                        HelpLine line;
                        line.left = std::string(sub.name);
                        if (!sub.aliases.empty())
                        {
                            line.left += " (";
                            for (size_t i = 0; i < sub.aliases.size(); i++)
                            {
                                if (i > 0)
                                    line.left += ", ";
                                line.left += sub.aliases[i];
                            }
                            line.left += ")";
                        }
                        line.right = std::string(sub.description);
                        lines.push_back(std::move(line));
                    }
                }));
        }

        return doc;
    }

    // ── format_help(HelpDocument) ──

    std::string HelpFormatter::format_help(const HelpDocument &doc)
    {
        std::ostringstream os;

        // Usage line
        os << format_usage(doc.usage) << "\n\n";

        // Description
        if (!doc.description.empty())
            os << doc.description << "\n\n";

        // Sections
        for (const auto &section : doc.sections)
        {
            os << section.heading << ":\n";
            size_t max_left = 0;
            size_t width_limit = (section.heading == "Options") ? size_t{32} : size_t{28};
            for (const auto &line : section.lines)
                max_left = std::max(max_left, line.left.size());
            size_t left_width = std::min(max_left, width_limit);
            for (const auto &line : section.lines)
                append_help_line(os, line.left, line.right, left_width);
            os << "\n";
        }

        return os.str();
    }

    // ── format_help(HelpInfo) — adapter ──

    std::string HelpFormatter::format_help(const HelpInfo &info)
    {
        return format_help(build_document(info));
    }

    // ── build_usage ──

    UsageInfo HelpFormatter::build_usage(const HelpInfo &info)
    {
        UsageInfo usage;
        usage.program_name = info.program_name;

        for (const auto &opt : info.options)
        {
            UsageToken tok;
            tok.display = option_label(opt, "|");
            if (!opt.is_required)
                tok.display = "[" + tok.display + "]";
            usage.tokens.push_back(std::move(tok));
        }

        for (const auto &arg : info.args)
        {
            UsageToken tok;
            if (arg.is_required)
                tok.display = "<" + std::string(arg.name) + ">";
            else
                tok.display = "[" + std::string(arg.name) + "]";
            usage.tokens.push_back(std::move(tok));
        }

        if (!info.subcommands.empty())
            usage.tokens.push_back({"<command>"});

        return usage;
    }

    // ── format_usage(UsageInfo) ──

    std::string HelpFormatter::format_usage(const UsageInfo &info)
    {
        std::ostringstream os;
        os << "Usage: ";
        if (!info.program_name.empty())
            os << info.program_name;
        else
            os << "app";
        for (const auto &tok : info.tokens) os << " " << tok.display;
        return os.str();
    }

    // ── format_usage(BaseCommand) — adapter ──

    std::string HelpFormatter::format_usage(
        const BaseCommand &cmd, std::string_view program_name)
    {
        auto info = collect_help(cmd, program_name);
        return format_usage(build_usage(info));
    }

    // ── format_help(BaseCommand) — adapter ──

    std::string HelpFormatter::format_help(
        const BaseCommand &cmd, std::string_view program_name)
    {
        return format_help(collect_help(cmd, program_name));
    }

}  // namespace pjh::cli
