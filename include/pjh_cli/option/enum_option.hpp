#ifndef INCLUDE_PJH_CLI_OPTION_ENUM_OPTION_HPP
#define INCLUDE_PJH_CLI_OPTION_ENUM_OPTION_HPP

#include <concepts>
#include <pjh_cli/core/error.hpp>
#include <pjh_cli/core/type.hpp>
#include <pjh_cli/option/mixin/with_default.hpp>
#include <pjh_cli/option/mixin/with_env.hpp>
#include <pjh_cli/option/mixin/with_repeatable.hpp>
#include <pjh_cli/option/mixin/with_required.hpp>
#include <pjh_cli/option/option_chain.hpp>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace pjh::cli
{
    namespace detail
    {
        /// @brief Enum types whose value fits the `int`-backed option storage.
        ///
        /// `EnumOption<E>` stores values as `int`, so an enum whose underlying
        /// type is wider than `int` would be silently truncated on conversion.
        /// This concept names that constraint so the policy is testable
        /// independently of the class template.
        template <typename E>
        concept IntStorableEnum = std::is_enum_v<E> && (sizeof(E) <= sizeof(int));
    }  // namespace detail

    /// @brief Enum-valued option with string-to-enum mapping.
    ///
    /// Maps CLI input strings to C++ enum values.  Stores values internally
    /// as `int`, so @p E must satisfy `detail::IntStorableEnum` (its
    /// underlying type is at most `int`-sized); an enum wider than `int` is a
    /// compile-time error instead of a silent truncation.  Same-size unsigned
    /// underlying types round-trip because the conversion is modulo `2^N` and
    /// `static_cast<E>` inverts it.  Retrieval via `ctx.get_enum<E, Key>()`
    /// provides type-safe access.
    ///
    /// Usage:
    /// @code
    ///   enum class Color { red, green, blue };
    ///   app.option<"color">("--color", 'c', "Color")
    ///       .enum_type<Color>()
    ///       .mapping({{"red", Color::red}, {"green", Color::green}})
    ///       .default_value(Color::red);
    ///   auto v = ctx.get_enum<Color, "color">();
    /// @endcode
    template <typename E>
        requires std::is_enum_v<E>
    class EnumOption : public detail::option_chain<
                           int,
                           EnumOption<E>,
                           WithRequired,
                           WithEnv,
                           WithRepeatable,
                           WithDefault>
    {
        static_assert(
            detail::IntStorableEnum<E>,
            "EnumOption stores values as int; an enum wider than int would "
            "be silently truncated. Use an int-backed enum.");

        using DefaultBase = WithDefault<int, EnumOption<E>>;
        struct Mapping
        {
            std::string name;
            E value;
        };
        std::vector<Mapping> m_mappings;

    protected:
        CliResult<int> convert_value(std::string_view raw) const override
        {
            for (auto &m : m_mappings)
                if (m.name == raw)
                    return CliResult<int>::Ok(static_cast<int>(m.value));
            std::vector<std::string> names;
            names.reserve(m_mappings.size());
            for (auto &m : m_mappings) names.push_back(m.name);
            return CliFailure{
                ErrorFactory::enum_value_error(this->display_name(), raw, names)};
        }

        std::string default_value_str() const override
        {
            if (this->m_default.is_some())
            {
                int v = this->m_default.unwrap();
                for (auto &m : m_mappings)
                    if (static_cast<int>(m.value) == v)
                        return m.name;
            }
            return "";
        }

    public:
        /// @brief Register the string-to-enum mapping.
        /// @param m List of {string_name, enum_value} pairs.
        EnumOption &mapping(std::vector<std::pair<std::string, E>> m)
        {
            m_mappings.clear();
            m_mappings.reserve(m.size());
            for (auto &p : m) m_mappings.push_back({std::move(p.first), p.second});
            return *this;
        }

        /// @brief Set a default enum value (convenience — casts to int).
        EnumOption &default_value(E v)
        {
            DefaultBase::default_value(static_cast<int>(v));
            return *this;
        }
    };

}  // namespace pjh::cli

#endif
