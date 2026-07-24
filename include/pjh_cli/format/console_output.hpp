#ifndef INCLUDE_PJH_CLI_CONSOLE_OUTPUT_HPP
#define INCLUDE_PJH_CLI_CONSOLE_OUTPUT_HPP

#include <pjh_cli/format/info.hpp>
#include <string>
#include <string_view>
#include <vector>

namespace pjh::cli
{

    class ConsoleOutput
    {
    public:
        ConsoleOutput() = delete;

        static std::string format_suggestions(const SuggestionInfo &info);

        static std::string format_no_subcommands();

        static std::string format_subcommand_list(
            const std::vector<std::string> &names);

        static std::string format_matched_subcommands(
            const std::vector<std::string> &names);

        static std::string format_no_match(const std::string &usage);

        static std::string format_has_no_subcommands(const std::string &name);

        static std::string format_unknown_subcommand(
            const std::string &name, std::string_view suggestions);
    };

}  // namespace pjh::cli

#endif