#ifndef INCLUDE_PJH_CLI_TYPE_HPP
#define INCLUDE_PJH_CLI_TYPE_HPP

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <pjh_result.hpp>
#include <string>

#include <pjh_cli/core/error.hpp>

namespace pjh::cli
{
    /// @brief Result type wrapping pjh::result::Result with CliError.
    ///
    /// Used throughout the library as the return type for all fallible
    /// operations (parsing, validation, execution).
    /// @tparam T Success type (may be void).
    template <typename T>
    using CliResult = pjh::result::Result<T, CliError>;

    /// @brief Convenience alias for returning an error from a CliResult function.
    ///
    /// Implicitly converts to any CliResult<T> with matching error type.
    ///
    /// Usage: `return CliFailure{ErrorFactory::unknown_option("--foo")};`
    using CliFailure = pjh::result::Failure<CliError>;

    /// @brief Runtime type tag for the five builtin option types.
    ///
    /// Used for runtime dispatch in apply_arg_value() and type_name().
    /// Maps 1:1 with the types satisfying the BuiltinType concept.
    enum class ValueTag : uint8_t
    {
        Bool,    ///< bool flag / negatable option
        Int,     ///< int option (also used by counting flags)
        Double,  ///< double / float option
        String,  ///< std::string option
        Path,    ///< std::filesystem::path option
    };

    namespace detail
    {
        /// @brief Concept: one of the five types supported by the option system.
        template <typename T>
        concept BuiltinType =
            std::same_as<T, bool> || std::same_as<T, int> || std::same_as<T, double> ||
            std::same_as<T, std::string> || std::same_as<T, std::filesystem::path>;

        /// @brief Compile-time mapping from C++ type to ValueTag.
        ///
        /// Used by LeafCommand::arg() and OptionBuilder to set the runtime
        /// type tag based on the template type argument.
        /// @tparam T A type satisfying BuiltinType.
        template <BuiltinType T>
        inline constexpr ValueTag value_tag_v =
            std::same_as<T, bool>                    ? ValueTag::Bool
            : std::same_as<T, int>                   ? ValueTag::Int
            : std::same_as<T, double>                ? ValueTag::Double
            : std::same_as<T, std::string>           ? ValueTag::String
            : std::same_as<T, std::filesystem::path> ? ValueTag::Path
                                                     : ValueTag::Bool;

        /// @brief Compile-time 0..4 index for each BuiltinType, used with
        ///        std::get<N> on the tuple storage in ParseContext.
        /// @tparam T A type satisfying BuiltinType.
        template <BuiltinType T>
        inline constexpr size_t type_index_v =
            std::same_as<T, bool>                    ? 0
            : std::same_as<T, int>                   ? 1
            : std::same_as<T, double>                ? 2
            : std::same_as<T, std::string>           ? 3
            : std::same_as<T, std::filesystem::path> ? 4
                                                     : 0;
    }

}  // namespace pjh::cli

#endif  // INCLUDE_PJH_CLI_TYPE_HPP
