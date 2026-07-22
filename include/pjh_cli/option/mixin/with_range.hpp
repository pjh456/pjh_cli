#ifndef INCLUDE_PJH_CLI_OPTION_MIXIN_WITH_RANGE_HPP
#define INCLUDE_PJH_CLI_OPTION_MIXIN_WITH_RANGE_HPP

#include <concepts>
#include <limits>
#include <pjh_result.hpp>
#include <string_view>
#include <type_traits>
#include <utility>

#include <pjh_cli/converter.hpp>
#include <pjh_cli/error.hpp>
#include <pjh_cli/option_def.hpp>
#include <pjh_cli/type.hpp>

namespace pjh::cli::detail
{

    template <typename T>
    concept NumericOptionType = detail::BuiltinType<T> && std::is_arithmetic_v<T>;

    /// @brief Compute an upper bound for error display when only min is set.
    /// @tparam T Numeric type.
    /// @param v The min bound value.
    /// @return v+1 for floats, numeric_limits::max() for integers.
    template <NumericOptionType T>
    T range_upper(const T &v) noexcept
    {
        if constexpr (std::is_floating_point_v<T>)
            return v + static_cast<T>(1);
        else
            return std::numeric_limits<T>::max();
    }

    /// @brief Compute a lower bound for error display when only max is set.
    /// @tparam T Numeric type.
    /// @param v The max bound value.
    /// @return v-1 for floats, numeric_limits::lowest() for integers.
    template <NumericOptionType T>
    T range_lower(const T &v) noexcept
    {
        if constexpr (std::is_floating_point_v<T>)
            return v - static_cast<T>(1);
        else
            return std::numeric_limits<T>::lowest();
    }

}  // namespace pjh::cli::detail

namespace pjh::cli
{

    /// @brief Mixin: adds numeric range validation (min / max).
    ///
    /// Overrides `convert_value` (using Converter<T>) and `validate_value`
    /// (checking min/max bounds).  Calls Base::validate_value to chain
    /// downstream validation (e.g. WithChoices).
    template <typename T, typename Derived, typename Base>
        requires detail::NumericOptionType<T> && std::derived_from<Base, OptionDef>
    class WithRange : public Base
    {
    protected:
        pjh::result::Option<T> m_min = pjh::result::Option<T>::None();
        pjh::result::Option<T> m_max = pjh::result::Option<T>::None();

    public:
        /// @brief Set the inclusive minimum value.
        /// @param v Lower bound (inclusive).
        /// @return *this for chaining.
        Derived &min(T v)
        {
            m_min = pjh::result::Option<T>::Some(std::move(v));
            return static_cast<Derived &>(*this);
        }

        /// @brief Set the inclusive maximum value.
        /// @param v Upper bound (inclusive).
        /// @return *this for chaining.
        Derived &max(T v)
        {
            m_max = pjh::result::Option<T>::Some(std::move(v));
            return static_cast<Derived &>(*this);
        }

    protected:
        CliResult<T> convert_value(std::string_view raw) const override
        {
            return Converter<T>::from_string(raw);
        }

        CliResult<void> validate_value(const T &v, std::string_view raw) const override
        {
            if (this->m_min.is_some() && v < this->m_min.unwrap())
                return CliFailure{ErrorFactory::value_out_of_range(
                    this->m_long_name, raw, this->m_min.unwrap(),
                    this->m_max.is_some() ? this->m_max.unwrap()
                                          : detail::range_upper<T>(v))};
            if (this->m_max.is_some() && v > this->m_max.unwrap())
                return CliFailure{ErrorFactory::value_out_of_range(
                    this->m_long_name, raw,
                    this->m_min.is_some() ? this->m_min.unwrap()
                                          : detail::range_lower<T>(v),
                    this->m_max.unwrap())};
            return Base::validate_value(v, raw);
        }
    };

}  // namespace pjh::cli

#endif
