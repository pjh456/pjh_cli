#ifndef INCLUDE_PJH_CLI_OPTION_MIXIN_WITH_DEFAULT_HPP
#define INCLUDE_PJH_CLI_OPTION_MIXIN_WITH_DEFAULT_HPP

#include <concepts>
#include <filesystem>
#include <pjh_cli/core/type.hpp>
#include <pjh_cli/option/option_def.hpp>
#include <pjh_result.hpp>
#include <string>
#include <string_view>
#include <utility>

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

    /// @brief Mixin: adds default value support + parse-value pipeline.
    ///
    /// Defines the `parse_value` pipeline as a non-overridable final method that
    /// delegates to two virtual hooks:
    ///   - `convert_value(raw)`  — string → T conversion
    ///   - `validate_value(v, raw)` — post-conversion validation (chained)
    ///
    /// Subclasses and downstream mixins override the hooks instead of `parse_value`.
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

        /// @brief Parse pipeline:  convert → validate → store.  Not overridable.
        CliResult<void> parse_value(
            ParseContext &ctx, std::string_view raw) const final override
        {
            auto r = convert_value(raw);
            if (r.is_err())
                return CliResult<void>::Err(std::move(r).unwrap_err());
            auto vr = validate_value(r.unwrap(), raw);
            if (vr.is_err())
                return vr;
            return this->store_or_append(ctx, this->m_key_hash, std::move(r.unwrap()));
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

    protected:
        /// @brief Convert raw string → typed value. Override in value-owning mixins.
        virtual CliResult<T> convert_value(std::string_view) const
        {
            return CliFailure{CliError("option does not accept a value")};
        }

        /// @brief Validate a parsed typed value. Chain by calling Base::validate_value.
        virtual CliResult<void> validate_value(const T &, std::string_view) const
        {
            return CliResult<void>::Ok();
        }
    };

}  // namespace pjh::cli

#endif
