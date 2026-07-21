#ifndef INCLUDE_PJH_CLI_OPTION_MIXIN_WITH_DEFAULT_HPP
#define INCLUDE_PJH_CLI_OPTION_MIXIN_WITH_DEFAULT_HPP

#include <concepts>
#include <filesystem>
#include <pjh_result.hpp>
#include <string>
#include <utility>

#include "pjh_cli/option_def.hpp"
#include "pjh_cli/type.hpp"

namespace pjh::cli::detail
{
    template <typename T>
    std::string format_for_help(const T &v);

    template <>
    inline std::string format_for_help<int>(const int &v)
    {
        return std::to_string(v);
    }

    template <>
    inline std::string format_for_help<double>(const double &v)
    {
        return std::to_string(v);
    }

    template <>
    inline std::string format_for_help<bool>(const bool &v)
    {
        return v ? "true" : "false";
    }

    template <>
    inline std::string format_for_help<std::string>(const std::string &v)
    {
        return v;
    }

    template <>
    inline std::string format_for_help<std::filesystem::path>(
        const std::filesystem::path &v)
    {
        return v.string();
    }

}  // namespace pjh::cli::detail

namespace pjh::cli
{

    /// @brief Mixin: adds default value support to an OptionDef-derived class.
    /// @tparam T       Stored value type (must satisfy BuiltinType).
    /// @tparam Derived Concrete class (CRTP).
    /// @tparam Base    Base class to extend (defaults to OptionDef).
    template <typename T, typename Derived, typename Base = OptionDef>
        requires detail::BuiltinType<T> && std::derived_from<Base, OptionDef>
    class WithDefault : public Base
    {
    protected:
        pjh::result::Option<T> m_default = pjh::result::Option<T>::None();

    public:
        bool has_default() const noexcept override { return m_default.is_some(); }

        std::string default_value_str() const override
        {
            if (m_default.is_some())
                return detail::format_for_help<T>(m_default.unwrap());
            return "";
        }

        CliResult<void> apply_default(ParseContext &ctx) const override
        {
            if (m_default.is_some() && !ctx.has_value(this->m_key_hash))
                return this->store_or_append(ctx, this->m_key_hash, m_default.unwrap());
            return CliResult<void>::Ok();
        }

        Derived &default_value(T v)
        {
            m_default = pjh::result::Option<T>::Some(std::move(v));
            return static_cast<Derived &>(*this);
        }
    };

}  // namespace pjh::cli

#endif
