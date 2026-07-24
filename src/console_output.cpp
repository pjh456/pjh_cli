#include <format>
#include <pjh_cli/format/console_output.hpp>
#include <string>
#include <string_view>
#include <vector>

namespace pjh::cli
{

    std::string ConsoleOutput::format_suggestions(const SuggestionInfo &info)
    {
        std::string out;
        for (auto &m : info.matches)
            out += " " + m.name;
        return out;
    }

    std::string ConsoleOutput::format_no_subcommands()
    {
        return "No subcommands available.";
    }

    std::string ConsoleOutput::format_subcommand_list(
        const std::vector<std::string> &names)
    {
        if (names.empty())
            return format_no_subcommands();
        std::string out = "Subcommands:";
        for (auto &n : names)
            out += " " + n;
        return out;
    }

    std::string ConsoleOutput::format_matched_subcommands(
        const std::vector<std::string> &names)
    {
        std::string out = "Matching subcommands:";
        for (auto &n : names)
            out += " " + n;
        return out;
    }

    std::string ConsoleOutput::format_no_match(const std::string &usage)
    {
        return std::format("No matches. Try: {}", usage);
    }

    std::string ConsoleOutput::format_has_no_subcommands(
        const std::string &name)
    {
        return std::format("'{}' has no subcommands.", name);
    }

    std::string ConsoleOutput::format_unknown_subcommand(
        const std::string &name, std::string_view suggestions)
    {
        if (suggestions.empty())
            return std::format("Unknown subcommand '{}'.", name);
        return std::format(
            "Unknown subcommand '{}'. Did you mean:{}", name, suggestions);
    }

}  // namespace pjh::cli