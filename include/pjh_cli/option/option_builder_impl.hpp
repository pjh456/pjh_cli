/// @file
/// Method definitions for OptionBuilder<Key>.
///
/// Self-contained with respect to the typed option classes it constructs.
/// `command/command_builder.hpp` includes base_command.hpp first so that
/// BaseCommand::add_option() is complete when these templates are parsed.

#ifndef INCLUDE_PJH_CLI_OPTION_BUILDER_IMPL_HPP
#define INCLUDE_PJH_CLI_OPTION_BUILDER_IMPL_HPP

#include <filesystem>
#include <memory>
#include <pjh_cli/core/fixed_string.hpp>
#include <pjh_cli/option/bool_option.hpp>
#include <pjh_cli/option/count_option.hpp>
#include <pjh_cli/option/enum_option.hpp>
#include <pjh_cli/option/float_option.hpp>
#include <pjh_cli/option/int_option.hpp>
#include <pjh_cli/option/option_builder.hpp>
#include <pjh_cli/option/path_option.hpp>
#include <pjh_cli/option/str_option.hpp>

namespace pjh::cli
{

    template <auto Key>
        requires detail::OptionKey<decltype(Key)>
    template <typename Opt>
    Opt &OptionBuilder<Key>::make_option(bool has_val)
    {
        auto name = m_long_name;
        if (name.size() > 2 && name[0] == '-' && name[1] == '-')
            name = name.substr(2);

        auto ptr = std::make_unique<Opt>();
        ptr->set_long_name(std::move(name));
        ptr->set_short_name(m_short_name);
        ptr->set_description(std::move(m_description));
        ptr->set_has_value(has_val);
        ptr->set_value_tag(detail::value_tag_v<typename Opt::ValueType>);
        ptr->set_key_hash(key_hash(Key));

        auto &ref = *ptr;
        m_cmd.add_option(std::move(ptr));
        return ref;
    }

    template <auto Key>
        requires detail::OptionKey<decltype(Key)>
    IntOption &OptionBuilder<Key>::integer()
    {
        return make_option<IntOption>(true);
    }

    template <auto Key>
        requires detail::OptionKey<decltype(Key)>
    CountOption &OptionBuilder<Key>::count()
    {
        return make_option<CountOption>(false);
    }

    template <auto Key>
        requires detail::OptionKey<decltype(Key)>
    BoolOption &OptionBuilder<Key>::boolean()
    {
        return make_option<BoolOption>(false);
    }

    template <auto Key>
        requires detail::OptionKey<decltype(Key)>
    StrOption &OptionBuilder<Key>::str()
    {
        return make_option<StrOption>(true);
    }

    template <auto Key>
        requires detail::OptionKey<decltype(Key)>
    FloatOption &OptionBuilder<Key>::floating()
    {
        return make_option<FloatOption>(true);
    }

    template <auto Key>
        requires detail::OptionKey<decltype(Key)>
    PathOption &OptionBuilder<Key>::path()
    {
        return make_option<PathOption>(true);
    }

    template <auto Key>
        requires detail::OptionKey<decltype(Key)>
    template <typename E>
        requires std::is_enum_v<E>
    EnumOption<E> &OptionBuilder<Key>::enum_type()
    {
        return make_option<EnumOption<E>>(true);
    }

}  // namespace pjh::cli

#endif  // INCLUDE_PJH_CLI_OPTION_BUILDER_IMPL_HPP
