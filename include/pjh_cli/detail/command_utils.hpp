#ifndef INCLUDE_PJH_CLI_DETAIL_COMMAND_UTILS_HPP
#define INCLUDE_PJH_CLI_DETAIL_COMMAND_UTILS_HPP

#include <format>
#include <string>
#include <string_view>

#include "../command/base_command.hpp"
#include "../option_def.hpp"
#include "string_utils.hpp"

namespace pjh::cli::detail
{

    inline bool is_visible_and_enabled(const BaseCommand &cmd, Visibility mode) noexcept
    {
        if (!cmd.is_enabled())
            return false;
        if ((cmd.visibility() & mode) == Visibility::Hidden)
            return false;
        return true;
    }

    inline std::string option_left_label(
        const OptionDef &opt, std::string_view sep = ", ")
    {
        std::string left;
        if (opt.short_name() != 0)
            left = std::format("-{}", opt.short_name());
        if (opt.short_name() != 0 && !opt.long_name().empty())
            left += sep;
        if (!opt.long_name().empty())
            left += "--" + opt.long_name();
        if (opt.has_value())
        {
            auto label = opt.long_name().empty() ? std::string(1, opt.short_name())
                                                 : opt.long_name();
            left += " " + StringUtils::to_upper_copy(label);
        }
        return left;
    }

}  // namespace pjh::cli::detail

#endif
