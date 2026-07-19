#ifndef INCLUDE_PJH_CLI_OPTION_PATH_OPTION_HPP
#define INCLUDE_PJH_CLI_OPTION_PATH_OPTION_HPP

#include <filesystem>

#include "../option_def.hpp"
#include "../parse_context.hpp"

namespace pjh::cli
{
    /// @brief Filesystem path option.
    class PathOption : public OptionDef
    {
    public:
        PathOption() : m_default(pjh::result::Option<std::filesystem::path>::None()) {}

        bool has_default() const noexcept override { return m_default.is_some(); }

        CliResult<void> parse_value(
            ParseContext &ctx, std::string_view raw) const override
        {
            ctx.set_value<std::filesystem::path>(m_key_hash, std::filesystem::path(raw));
            return CliResult<void>::Ok();
        }

        CliResult<void> apply_default(ParseContext &ctx) const override
        {
            if (m_default.is_some() && !ctx.has_value(m_key_hash))
                ctx.set_value<std::filesystem::path>(m_key_hash, m_default.unwrap());
            return CliResult<void>::Ok();
        }

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
