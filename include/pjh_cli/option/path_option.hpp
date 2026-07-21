#ifndef INCLUDE_PJH_CLI_OPTION_PATH_OPTION_HPP
#define INCLUDE_PJH_CLI_OPTION_PATH_OPTION_HPP

#include <filesystem>
#include <string_view>

#include "mixin/with_default.hpp"
#include "pjh_cli/option_def.hpp"
#include "pjh_cli/type.hpp"

namespace pjh::cli
{

    /// @brief Filesystem path option.
    class PathOption
        : public WithDefault<std::filesystem::path, PathOption>
    {
    public:
        CliResult<void> parse_value(
            ParseContext &ctx, std::string_view raw) const override
        {
            return store_or_append(ctx, m_key_hash, std::filesystem::path(raw));
        }
    };

}  // namespace pjh::cli

#endif
