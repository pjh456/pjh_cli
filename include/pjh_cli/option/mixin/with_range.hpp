#ifndef INCLUDE_PJH_CLI_OPTION_MIXIN_WITH_RANGE_HPP
#define INCLUDE_PJH_CLI_OPTION_MIXIN_WITH_RANGE_HPP

#include <concepts>
#include <limits>
#include <pjh_result.hpp>
#include <string_view>
#include <type_traits>
#include <utility>

#include "pjh_cli/converter.hpp"
#include "pjh_cli/error.hpp"
#include "pjh_cli/option_def.hpp"
#include "pjh_cli/type.hpp"

namespace pjh::cli::detail
{

    template <typename T>
    concept NumericOptionType = detail::BuiltinType<T> && std::is_arithmetic_v<T>;

    template <NumericOptionType T>
    T range_upper(const T &v) noexcept
    {
        if constexpr (std::is_floating_point_v<T>)
            return v + static_cast<T>(1);
        else
            return std::numeric_limits<T>::max();
    }

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

    /// @brief Mixin: adds min/max range validation to an OptionDef-derived class.
    /// @tparam T       Numeric type (int, double).
    /// @tparam Derived Concrete class (CRTP).
    /// @tparam Base    Base class to extend (required).
    template <typename T, typename Derived, typename Base>
        requires detail::NumericOptionType<T> && std::derived_from<Base, OptionDef>
    class WithRange : public Base
    {
    protected:
        pjh::result::Option<T> m_min = pjh::result::Option<T>::None();
        pjh::result::Option<T> m_max = pjh::result::Option<T>::None();

    public:
        CliResult<void> parse_value(
            ParseContext &ctx, std::string_view raw) const override
        {
            auto r = Converter<T>::from_string(raw);
            if (r.is_err())
                return CliResult<void>::Err(std::move(r).unwrap_err());
            T v = r.unwrap();
            if (this->m_min.is_some() && v < this->m_min.unwrap())
                return CliFailure{value_out_of_range(
                    this->m_long_name, raw, this->m_min.unwrap(),
                    this->m_max.is_some() ? this->m_max.unwrap()
                                          : detail::range_upper<T>(v))};
            if (this->m_max.is_some() && v > this->m_max.unwrap())
                return CliFailure{value_out_of_range(
                    this->m_long_name, raw,
                    this->m_min.is_some() ? this->m_min.unwrap()
                                          : detail::range_lower<T>(v),
                    this->m_max.unwrap())};
            return this->store_or_append(ctx, this->m_key_hash, v);
        }

        Derived &min(T v)
        {
            m_min = pjh::result::Option<T>::Some(std::move(v));
            return static_cast<Derived &>(*this);
        }

        Derived &max(T v)
        {
            m_max = pjh::result::Option<T>::Some(std::move(v));
            return static_cast<Derived &>(*this);
        }
    };

}  // namespace pjh::cli

#endif
