#ifndef INCLUDE_PJH_CLI_OPTION_MIXIN_WITH_REPEATABLE_HPP
#define INCLUDE_PJH_CLI_OPTION_MIXIN_WITH_REPEATABLE_HPP

#include <pjh_cli/option/option_def.hpp>
#include <type_traits>

namespace pjh::cli
{

    /// @brief Mixin: adds repeatable/multi-value capability.
    ///
    /// When repeatable is enabled, the option consumes multiple value tokens
    /// greedily (--files a b c) and appends them to a vector.  Repeated flag
    /// occurrences (--files a --files b) also append.
    template <typename T, typename Derived, typename Base = OptionDef>
        requires std::derived_from<Base, OptionDef>
    class WithRepeatable : public Base
    {
        bool m_repeatable = false;

    public:
        bool is_repeatable() const noexcept override { return m_repeatable; }

        Derived &repeatable()
        {
            m_repeatable = true;
            return static_cast<Derived &>(*this);
        }
    };

}  // namespace pjh::cli

#endif
