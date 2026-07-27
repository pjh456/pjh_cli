/// @file
/// Method definitions for OptionGroupBuilder<Keys...>.
///
/// This file is included at the bottom of base_command.hpp so that
/// BaseCommand::find_option_by_hash() and register_group() are fully visible.

#ifndef INCLUDE_PJH_CLI_OPTION_GROUP_BUILDER_IMPL_HPP
#define INCLUDE_PJH_CLI_OPTION_GROUP_BUILDER_IMPL_HPP

#include <pjh_cli/core/fixed_string.hpp>
#include <pjh_cli/option/option_group_builder.hpp>

namespace pjh::cli
{

    template <auto... Keys>
        requires(... && detail::OptionKey<decltype(Keys)>)
    void OptionGroupBuilder<Keys...>::commit(GroupMode mode)
    {
        OptionGroup g;
        g.mode = mode;
        g.key_hashes = {key_hash(Keys)...};
        g.option_names.reserve(g.key_hashes.size());
        for (auto h : g.key_hashes)
        {
            auto *opt = m_cmd.find_option_by_hash(h);
            g.option_names.push_back(opt ? "--" + opt->long_name() : "?");
        }
        m_cmd.register_group(std::move(g));
    }

    template <auto... Keys>
        requires(... && detail::OptionKey<decltype(Keys)>)
    OptionGroupBuilder<Keys...> &OptionGroupBuilder<Keys...>::exactly_one()
    {
        commit(GroupMode::ExactlyOne);
        return *this;
    }

    template <auto... Keys>
        requires(... && detail::OptionKey<decltype(Keys)>)
    OptionGroupBuilder<Keys...> &OptionGroupBuilder<Keys...>::at_most_one()
    {
        commit(GroupMode::AtMostOne);
        return *this;
    }

    template <auto... Keys>
        requires(... && detail::OptionKey<decltype(Keys)>)
    OptionGroupBuilder<Keys...> &OptionGroupBuilder<Keys...>::at_least_one()
    {
        commit(GroupMode::AtLeastOne);
        return *this;
    }

}  // namespace pjh::cli

#endif  // INCLUDE_PJH_CLI_OPTION_GROUP_BUILDER_IMPL_HPP
