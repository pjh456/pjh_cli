#ifndef INCLUDE_PJH_CLI_OPTION_MIXIN_WITH_CHOICES_HPP
#define INCLUDE_PJH_CLI_OPTION_MIXIN_WITH_CHOICES_HPP

#include <concepts>
#include <filesystem>
#include <ranges>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "pjh_cli/error.hpp"
#include "pjh_cli/option_def.hpp"
#include "pjh_cli/type.hpp"

namespace pjh::cli::detail
{

    template <typename T>
    std::string choice_key(const T &v);

    template <>
    inline std::string choice_key<int>(const int &v)
    {
        return std::to_string(v);
    }

    template <>
    inline std::string choice_key<double>(const double &v)
    {
        return std::to_string(v);
    }

    template <>
    inline std::string choice_key<bool>(const bool &v)
    {
        return v ? "true" : "false";
    }

    template <>
    inline std::string choice_key<std::string>(const std::string &v)
    {
        return v;
    }

    template <>
    inline std::string choice_key<std::filesystem::path>(const std::filesystem::path &v)
    {
        return v.string();
    }

}  // namespace pjh::cli::detail

namespace pjh::cli
{

    /// @brief Mixin: restricts values to an explicit set of allowed strings.
    ///
    /// Overrides `validate_value` to check the parsed value against the
    /// allowed choices list.  Calls Base::validate_value for further chaining.
    template <typename T, typename Derived, typename Base>
        requires detail::BuiltinType<T> && std::derived_from<Base, OptionDef>
    class WithChoices : public Base
    {
        std::vector<std::string> m_choices;

    public:
        Derived &choices(std::vector<std::string> vals)
        {
            m_choices = std::move(vals);
            return static_cast<Derived &>(*this);
        }

    protected:
        CliResult<void> validate_value(const T &v, std::string_view raw) const override
        {
            if (!m_choices.empty())
            {
                auto key = detail::choice_key<T>(v);
                if (std::ranges::find(m_choices, key) == m_choices.end())
                    return CliFailure{
                        ErrorFactory::invalid_choice(this->m_long_name, raw, m_choices)};
            }
            return Base::validate_value(v, raw);
        }
    };

}  // namespace pjh::cli

#endif
