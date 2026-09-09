#ifndef INCLUDE_PJH_CLI_OPTION_MIXIN_WITH_DEFAULT_HPP
#define INCLUDE_PJH_CLI_OPTION_MIXIN_WITH_DEFAULT_HPP

#include <concepts>
#include <pjh_cli/core/type.hpp>
#include <pjh_cli/option/option_def.hpp>
#include <pjh_result.hpp>
#include <string>
#include <string_view>
#include <utility>

namespace pjh::cli
{

    /// @brief Mixin: adds default value support + parse-value pipeline.
    ///
    /// Defines the `parse_value` pipeline as a non-overridable final method that
    /// delegates to two virtual hooks:
    ///   - `convert_value(raw)`  — string → T conversion
    ///   - `validate_value(v, raw)` — post-conversion validation (chained)
    ///
    /// The pipeline yields an OptionValue; the parse layer (ValueWriter)
    /// stores it.  Subclasses and downstream mixins override the hooks
    /// instead of `parse_value`.
    template <typename T, typename Derived, typename Base = OptionDef>
        requires detail::BuiltinType<T> && std::derived_from<Base, OptionDef>
    class WithDefault : public Base
    {
    protected:
        pjh::result::Option<T> m_default = pjh::result::Option<T>::None();

    public:
        /// @brief Storage type of this option (used by OptionBuilder to derive
        ///        the runtime ValueTag from detail::BuiltinTraits).
        using ValueType = T;

        bool has_default() const noexcept override { return m_default.is_some(); }

        /// @brief Render the default value for help.
        ///
        /// Delegates to detail::BuiltinTraits<T>::default_string so the
        /// display format has a single source of truth.
        std::string default_value_str() const override
        {
            if (m_default.is_some())
                return detail::BuiltinTraits<T>::default_string(m_default.unwrap());
            return "";
        }

        /// @brief Parse pipeline:  convert → validate → yield.  Not overridable.
        CliResult<OptionValue> parse_value(std::string_view raw) const final override
        {
            auto r = convert_value(raw);
            if (r.is_err())
                return CliResult<OptionValue>::Err(std::move(r).unwrap_err());
            auto vr = validate_value(r.unwrap(), raw);
            if (vr.is_err())
                return CliResult<OptionValue>::Err(std::move(vr).unwrap_err());
            return CliResult<OptionValue>::Ok(OptionValue{std::move(r.unwrap())});
        }

        /// @brief Yield the validated default value, if one is registered.
        ///
        /// Presence checks stay in the parse layer
        /// (ParseFinalizer::apply_defaults); validation errors propagate.
        CliResult<pjh::result::Option<OptionValue>> default_option_value() const override
        {
            if (m_default.is_none())
                return CliResult<pjh::result::Option<OptionValue>>::Ok(
                    pjh::result::Option<OptionValue>::None());
            auto vr = this->validate_value(m_default.unwrap(), "");
            if (vr.is_err())
                return CliResult<pjh::result::Option<OptionValue>>::Err(
                    std::move(vr).unwrap_err());
            return CliResult<pjh::result::Option<OptionValue>>::Ok(
                pjh::result::Option<OptionValue>::Some(OptionValue{m_default.unwrap()}));
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
            return CliFailure{
                ErrorFactory::option_does_not_accept_value(this->display_name())};
        }

        /// @brief Validate a parsed typed value. Chain by calling Base::validate_value.
        virtual CliResult<void> validate_value(const T &, std::string_view) const
        {
            return CliResult<void>::Ok();
        }
    };

}  // namespace pjh::cli

#endif
