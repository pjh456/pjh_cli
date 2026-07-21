#ifndef INCLUDE_PJH_CLI_HELP_FORMATTER_HPP
#define INCLUDE_PJH_CLI_HELP_FORMATTER_HPP

#include <string>
#include <string_view>

#include "command/base_command.hpp"
#include "info.hpp"

namespace pjh::cli
{

    class HelpFormatter
    {
    public:
        HelpFormatter() = delete;

        static std::string format_usage(
            const BaseCommand &cmd, std::string_view program_name = "");

        static std::string format_help(
            const BaseCommand &cmd, std::string_view program_name = "");

        static HelpInfo collect_help(
            const BaseCommand &cmd,
            std::string_view program_name = "",
            Visibility visibility = Visibility::Both);

        static std::string format_help(const HelpInfo &info);

    private:
        static void append_help_line(
            std::ostringstream &os,
            const std::string &left,
            const std::string &right,
            size_t left_width);

        static std::string option_label(
            const OptionInfo &opt, std::string_view sep = ", ");
    };

}  // namespace pjh::cli

#endif
