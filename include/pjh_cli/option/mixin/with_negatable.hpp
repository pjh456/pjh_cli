#ifndef INCLUDE_PJH_CLI_OPTION_MIXIN_WITH_NEGATABLE_HPP
#define INCLUDE_PJH_CLI_OPTION_MIXIN_WITH_NEGATABLE_HPP

#include <concepts>
#include <utility>

#include <pjh_cli/option/option_def.hpp>
#include <pjh_cli/core/type.hpp>

namespace pjh::cli
{

    /// @brief Mixin: adds --no-xxx negation support.
    ///
    /// When present, `is_negatable()` returns true and the parser
    /// recognises `--no-<long_name>` as a way to set the value to false.
    template <typename T, typename Derived, typename Base>
        requires detail::BuiltinType<T> && std::derived_from<Base, OptionDef>
    class WithNegatable : public Base
    {
        bool m_negatable = false;

    public:
        bool is_negatable() const noexcept override { return m_negatable; }

        Derived &negatable()
        {
            m_negatable = true;
            return static_cast<Derived &>(*this);
        }
    };

}  // namespace pjh::cli

#endif
