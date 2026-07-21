#ifndef INCLUDE_PJH_CLI_OPTION_PATH_OPTION_HPP
#define INCLUDE_PJH_CLI_OPTION_PATH_OPTION_HPP

#include <filesystem>
#include <string_view>

#include "mixin/with_default.hpp"

namespace pjh::cli
{

    /// @brief Filesystem path option.
    class PathOption : public WithDefault<std::filesystem::path, PathOption>
    {
    protected:
        CliResult<std::filesystem::path> convert_value(
            std::string_view raw) const override
        {
            return CliResult<std::filesystem::path>::Ok(std::filesystem::path(raw));
        }
    };

}  // namespace pjh::cli

#endif
