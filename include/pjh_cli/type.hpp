#ifndef INCLUDE_PJH_CLI_TYPE_HPP
#define INCLUDE_PJH_CLI_TYPE_HPP

#include <pjh_result.hpp>

#include "error.hpp"

#include <concepts>
#include <filesystem>
#include <type_traits>

namespace pjh::cli
{
    /// @brief Result type with CliError as the error variant.
    ///
    /// Wraps pjh::result::Result<T, CliError> for CLI parse operations.
    /// @tparam T Success type.
    template <typename T>
    using CliResult =
        pjh::result::Result<T, CliError>;

    /// @brief Convenience alias for returning CliError from Result-returning functions.
    ///
    /// Usage: `return CliFailure{CliError("...")};`
    /// Implicitly converts to any CliResult<T> with matching error type.
    using CliFailure =
        pjh::result::Failure<CliError>;

    /// @brief Runtime type tag for the five builtin option types.
    enum class ValueTag : uint8_t
    {
        Bool,
        Int,
        Double,
        String,
        Path,
    };

    namespace detail
    {
        template <typename T>
        concept BuiltinType =
            std::same_as<T, bool> ||
            std::same_as<T, int> ||
            std::same_as<T, double> ||
            std::same_as<T, std::string> ||
            std::same_as<T, std::filesystem::path>;

        /// @brief Compile-time ValueTag lookup for each BuiltinType.
        template <BuiltinType T>
        inline constexpr ValueTag value_tag_v =
            std::same_as<T, bool>                    ? ValueTag::Bool
            : std::same_as<T, int>                   ? ValueTag::Int
            : std::same_as<T, double>                ? ValueTag::Double
            : std::same_as<T, std::string>           ? ValueTag::String
            : std::same_as<T, std::filesystem::path> ? ValueTag::Path
                                                     : ValueTag::Bool;
    }

} // namespace pjh::cli

#endif // INCLUDE_PJH_CLI_TYPE_HPP
