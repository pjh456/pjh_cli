#ifndef INCLUDE_PJH_CLI_OPTION_PATH_OPTION_HPP
#define INCLUDE_PJH_CLI_OPTION_PATH_OPTION_HPP

#include <filesystem>
#include <pjh_result.hpp>
#include <string_view>
#include <utility>

#include "../option_def.hpp"
#include "../parse_context.hpp"
#include "pjh_cli/type.hpp"

namespace pjh::cli
{
    /// @brief Filesystem path option.
    ///
    /// Created by `OptionBuilder::path()`.  Overrides `parse_value()` and
    /// `apply_default()` to store values as `std::filesystem::path` in
    /// `ParseContext`.
    class PathOption : public OptionDef
    {
    public:
        /// @brief Construct with no default value.
        PathOption() : m_default(pjh::result::Option<std::filesystem::path>::None()) {}

        /// @brief Whether a default path value has been set.
        bool has_default() const noexcept override { return m_default.is_some(); }

        /// @brief Human-readable default value for help output.
        std::string default_value_str() const override
        {
            return m_default.is_some() ? m_default.unwrap().string() : "";
        }

        /// @brief Convert the raw string to a filesystem path and store.
        /// @param ctx Parse context.
        /// @param raw CLI token value (path string).
        /// @return Always Ok.
        CliResult<void> parse_value(
            ParseContext &ctx, std::string_view raw) const override
        {
            return store_or_append(ctx, m_key_hash, std::filesystem::path(raw));
        }

        /// @brief Apply the default path value if @p ctx has none.
        CliResult<void> apply_default(ParseContext &ctx) const override
        {
            if (m_default.is_some() && !ctx.has_value(m_key_hash))
                return store_or_append(ctx, m_key_hash, m_default.unwrap());
            return CliResult<void>::Ok();
        }

        /// @brief Register a typed default value.
        /// @param v Default path value.
        /// @return *this for chaining.
        PathOption &default_value(std::filesystem::path v)
        {
            m_default = decltype(m_default)::Some(std::move(v));
            return *this;
        }

    private:
        pjh::result::Option<std::filesystem::path> m_default;
    };

}  // namespace pjh::cli

#endif  // INCLUDE_PJH_CLI_OPTION_PATH_OPTION_HPP
