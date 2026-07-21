#ifndef INCLUDE_PJH_CLI_OPTION_BOOL_OPTION_HPP
#define INCLUDE_PJH_CLI_OPTION_BOOL_OPTION_HPP

#include "mixin/with_default.hpp"
#include "pjh_cli/option_def.hpp"
#include "pjh_cli/type.hpp"

namespace pjh::cli
{

    /// @brief Boolean flag option with optional --no-xxx negation.
    class BoolOption : public WithDefault<bool, BoolOption>
    {
        bool m_negatable = false;

    public:
        bool is_negatable() const noexcept override { return m_negatable; }

        BoolOption &negatable()
        {
            m_negatable = true;
            return *this;
        }
    };

}  // namespace pjh::cli

#endif
