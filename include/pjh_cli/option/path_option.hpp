#ifndef INCLUDE_PJH_CLI_OPTION_PATH_OPTION_HPP
#define INCLUDE_PJH_CLI_OPTION_PATH_OPTION_HPP

#include <filesystem>
#include <pjh_cli/option/mixin/with_default.hpp>
#include <pjh_cli/option/mixin/with_env.hpp>
#include <pjh_cli/option/mixin/with_repeatable.hpp>
#include <pjh_cli/option/mixin/with_required.hpp>
#include <string_view>

namespace pjh::cli
{

    /// @brief Filesystem path option.
    class PathOption : public WithRequired<
                           PathOption,
                           WithEnv<
                               PathOption,
                               WithRepeatable<
                                   PathOption,
                                   WithDefault<std::filesystem::path, PathOption>>>>
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
