#ifndef INCLUDE_PJH_CLI_OPTION_GROUP_BUILDER_HPP
#define INCLUDE_PJH_CLI_OPTION_GROUP_BUILDER_HPP

#include <pjh_cli/detail/concept.hpp>
#include <pjh_cli/option/group.hpp>

namespace pjh::cli
{

    class BaseCommand;

    /// @brief Builder returned by BaseCommand::group<Keys...>().
    ///
    /// Options are referenced by their compile-time key.  The group is
    /// validated after parsing: defaults and env-var values count as "set".
    ///
    /// Usage:
    /// @code
    ///   app.group<fixed_string("port"), fixed_string("socket")>().exactly_one();
    ///   app.group<fixed_string("verbose"), fixed_string("quiet")>().at_most_one();
    ///   app.group<fixed_string("f1"), fixed_string("f2")>().at_least_one();
    /// @endcode
    template <auto... Keys>
        requires(... && detail::OptionKey<decltype(Keys)>)
    class OptionGroupBuilder
    {
        BaseCommand &m_cmd;

        void commit(GroupMode mode);

    public:
        explicit OptionGroupBuilder(BaseCommand &cmd) : m_cmd(cmd) {}

        /// @brief Exactly one option in the group must be provided.
        OptionGroupBuilder &exactly_one();

        /// @brief Zero or one option in the group may be provided.
        OptionGroupBuilder &at_most_one();

        /// @brief At least one option in the group must be provided.
        OptionGroupBuilder &at_least_one();
    };

}  // namespace pjh::cli

#endif  // INCLUDE_PJH_CLI_OPTION_GROUP_BUILDER_HPP
