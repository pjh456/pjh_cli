#ifndef INCLUDE_PJH_CLI_DETAIL_COMMAND_UTILS_HPP
#define INCLUDE_PJH_CLI_DETAIL_COMMAND_UTILS_HPP

#include "../command.hpp"
#include "../option_def.hpp"
#include "string_utils.hpp"

namespace pjh::cli::detail
{

    inline bool
    is_visible_and_enabled(
        const Command &cmd,
        Visibility mode) noexcept
    {
        if (!cmd.is_enabled())
            return false;
        if ((cmd.visibility() & mode) == Visibility::Hidden)
            return false;
        return true;
    }

    inline std::string
    option_left_label(
        const OptionDef &opt,
        std::string_view sep = ", ")
    {
        std::string left;
        if (opt.m_short_name != 0)
            left = std::string("-") + opt.m_short_name;
        if (opt.m_short_name != 0 && !opt.m_long_name.empty())
            left += sep;
        if (!opt.m_long_name.empty())
            left += "--" + opt.m_long_name;
        if (opt.m_has_value)
        {
            auto label =
                opt.m_long_name.empty()
                    ? std::string(1, opt.m_short_name)
                    : opt.m_long_name;
            left += " " + to_upper_copy(label);
        }
        return left;
    }

} // namespace pjh::cli::detail

#endif
