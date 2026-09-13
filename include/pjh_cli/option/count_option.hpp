#ifndef INCLUDE_PJH_CLI_OPTION_COUNT_OPTION_HPP
#define INCLUDE_PJH_CLI_OPTION_COUNT_OPTION_HPP

#include <pjh_cli/core/converter.hpp>
#include <pjh_cli/option/mixin/with_default.hpp>
#include <pjh_cli/option/mixin/with_env.hpp>
#include <pjh_cli/option/mixin/with_required.hpp>
#include <pjh_cli/option/option_chain.hpp>
#include <string_view>

namespace pjh::cli
{

    /// @brief Counting-flag option (-vvv → 3).
    ///
    /// Never consumes a CLI value token; each occurrence increments the stored
    /// int by 1.  Shares the @c WithDefault<int> pipeline with every other kind,
    /// so an absent count can fall back to an environment variable (parsed as an
    /// integer) or a registered default value, with CLI > env > default
    /// precedence.  Accessible via `ctx.get<int, "key">()`.
    ///
    /// Note that a CLI occurrence replaces (not adds to) any env/default seed:
    /// a default of 5 with `-vv` yields 2.  Direct calls to
    /// @c parse_value("3") now convert successfully instead of being rejected;
    /// the CLI never takes that path because @c has_value() is false.
    class CountOption
        : public detail::
              option_chain<int, CountOption, WithRequired, WithEnv, WithDefault>
    {
    public:
        bool has_value() const noexcept override { return false; }

        bool is_counting() const noexcept override { return true; }

    protected:
        CliResult<int> convert_value(std::string_view raw) const override
        {
            return Converter<int>::from_string(raw, this->display_name());
        }
    };

}  // namespace pjh::cli

#endif  // INCLUDE_PJH_CLI_OPTION_COUNT_OPTION_HPP
