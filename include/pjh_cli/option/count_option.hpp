#ifndef INCLUDE_PJH_CLI_OPTION_COUNT_OPTION_HPP
#define INCLUDE_PJH_CLI_OPTION_COUNT_OPTION_HPP

#include <pjh_cli/core/error.hpp>
#include <pjh_cli/core/type.hpp>
#include <pjh_cli/option/mixin/with_required.hpp>
#include <pjh_cli/option/option_chain.hpp>
#include <pjh_cli/option/option_def.hpp>
#include <string_view>

namespace pjh::cli
{

    /// @brief Counting-flag option (-vvv → 3).
    ///
    /// Never consumes a CLI value token, never has a default value.
    /// Each occurrence increments the stored int by 1.
    /// Accessible via `ctx.get<int, "key">()`.
    class CountOption : public detail::option_chain<void, CountOption, WithRequired>
    {
    public:
        /// @brief Storage type used for the derived ValueTag (int).
        using ValueType = int;

        bool has_value() const noexcept override { return false; }

        bool has_default() const noexcept override { return false; }

        bool is_counting() const noexcept override { return true; }
    };

}  // namespace pjh::cli

#endif
