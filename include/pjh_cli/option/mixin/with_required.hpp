#ifndef INCLUDE_PJH_CLI_OPTION_MIXIN_WITH_REQUIRED_HPP
#define INCLUDE_PJH_CLI_OPTION_MIXIN_WITH_REQUIRED_HPP

#include <pjh_cli/option/option_def.hpp>
#include <type_traits>

namespace pjh::cli
{

    /// @brief Mixin: adds required-option support.
    ///
    /// When an option is marked required, parsing fails if the option
    /// does not appear on the command line.
    template <typename T, typename Derived, typename Base = OptionDef>
        requires std::derived_from<Base, OptionDef>
    class WithRequired : public Base
    {
        bool m_required = false;

    public:
        bool is_required() const noexcept override { return m_required; }

        /// @brief Mark this option as required.
        Derived &required(bool r = true)
        {
            m_required = r;
            return static_cast<Derived &>(*this);
        }
    };

}  // namespace pjh::cli

#endif
