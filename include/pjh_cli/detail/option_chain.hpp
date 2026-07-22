#ifndef INCLUDE_PJH_CLI_DETAIL_OPTION_CHAIN_HPP
#define INCLUDE_PJH_CLI_DETAIL_OPTION_CHAIN_HPP

#include <pjh_cli/option/option_def.hpp>

namespace pjh::cli::detail
{

    /// @brief Recursive mixin chain builder.
    ///
    /// Folds a variadic list of mixin templates into a nested inheritance chain.
    /// Base case: no mixins left, return @p Base.
    template <
        typename T,
        typename D,
        typename Base,
        template <typename, typename, typename...> typename... Mixins>
    struct chain;

    template <typename T, typename D, typename Base>
    struct chain<T, D, Base>
    {
        using type = Base;
    };

    template <
        typename T,
        typename D,
        typename Base,
        template <typename, typename, typename...> typename M,
        template <typename, typename, typename...> typename... Rest>
    struct chain<T, D, Base, M, Rest...>
    {
        using type = M<T, D, typename chain<T, D, Base, Rest...>::type>;
    };

    /// @brief Convenience alias that starts the chain from OptionDef.
    template <
        typename T,
        typename D,
        template <typename, typename, typename...> typename... Mixins>
    using option_chain = typename chain<T, D, OptionDef, Mixins...>::type;

}  // namespace pjh::cli::detail

#endif
