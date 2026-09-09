#ifndef INCLUDE_PJH_CLI_TYPE_HPP
#define INCLUDE_PJH_CLI_TYPE_HPP

#include <array>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <pjh_cli/core/error.hpp>
#include <pjh_result.hpp>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>

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

    /// @brief Runtime type tag for the builtin option types.
    ///
    /// Used for runtime dispatch in ValueWriter::apply_arg_value() and
    /// HintBuilder's type_name().  Each real enumerator corresponds to one
    /// detail::BuiltinTraits row; its ordinal must equal the storage index in
    /// detail::BuiltinTypes, which the static_asserts below enforce.  Count is
    /// a sentinel, never stored or indexed, and pins the enum cardinality so a
    /// stray enumerator is a compile-time error; it must stay last.
    enum class ValueTag : uint8_t
    {
        Bool,    ///< bool flag / negatable option
        Int,     ///< int option (also used by counting flags)
        Double,  ///< double / float option
        String,  ///< std::string option
        Path,    ///< std::filesystem::path option
        Count    ///< Sentinel: number of real tags; must stay last, never stored.
    };

    namespace detail
    {
        /// @brief Metadata row for one builtin storage type.
        ///
        /// The primary template is the "not a builtin" case.  The builtin
        /// types are specialized with the runtime ValueTag, the hint label and
        /// the default-value display used by help.  Conversion is deliberately
        /// not part of this row: it stays in Converter<T> (converter.hpp),
        /// which depends on this header.
        ///
        /// Specializing this template without also adding a BuiltinTypes entry
        /// is a bug: the ParseContext storage maps are generated from
        /// BuiltinTypes, not from this table.
        ///
        /// @tparam T Candidate storage type.
        template <typename T>
        struct BuiltinTraits
        {
            static constexpr bool is_builtin = false;
        };

        /// @brief Traits row for bool.
        template <>
        struct BuiltinTraits<bool>
        {
            static constexpr bool is_builtin = true;
            static constexpr ValueTag tag = ValueTag::Bool;
            static constexpr std::string_view hint_name = "BOOL";
            static std::string default_string(const bool &v)
            {
                return v ? "true" : "false";
            }
        };

        /// @brief Traits row for int.
        template <>
        struct BuiltinTraits<int>
        {
            static constexpr bool is_builtin = true;
            static constexpr ValueTag tag = ValueTag::Int;
            static constexpr std::string_view hint_name = "INT";
            static std::string default_string(const int &v) { return std::to_string(v); }
        };

        /// @brief Traits row for double.
        template <>
        struct BuiltinTraits<double>
        {
            static constexpr bool is_builtin = true;
            static constexpr ValueTag tag = ValueTag::Double;
            static constexpr std::string_view hint_name = "FLOAT";
            static std::string default_string(const double &v)
            {
                return std::to_string(v);
            }
        };

        /// @brief Traits row for std::string.
        template <>
        struct BuiltinTraits<std::string>
        {
            static constexpr bool is_builtin = true;
            static constexpr ValueTag tag = ValueTag::String;
            static constexpr std::string_view hint_name = "STR";
            static std::string default_string(const std::string &v) { return v; }
        };

        /// @brief Traits row for std::filesystem::path.
        template <>
        struct BuiltinTraits<std::filesystem::path>
        {
            static constexpr bool is_builtin = true;
            static constexpr ValueTag tag = ValueTag::Path;
            static constexpr std::string_view hint_name = "PATH";
            static std::string default_string(const std::filesystem::path &v)
            {
                return v.string();
            }
        };

        /// @brief Canonical ordered list of builtin storage types.
        ///
        /// Position in this tuple is the ParseContext storage index and must
        /// equal the ValueTag ordinal; the static_asserts below enforce that.
        /// Adding a storage type means adding one row here, one BuiltinTraits
        /// specialization, and one ValueTag enumerator before ValueTag::Count,
        /// plus a branch in detail::dispatch_default().
        using BuiltinTypes =
            std::tuple<bool, int, double, std::string, std::filesystem::path>;

        /// @brief Concept: one of the storage types supported by the option
        ///        system.
        /// @tparam T Candidate storage type.
        template <typename T>
        concept BuiltinType = BuiltinTraits<T>::is_builtin;

        /// @brief Compile-time mapping from C++ type to ValueTag.
        ///
        /// Used by LeafCommand::arg() and OptionBuilder to set the runtime
        /// type tag based on the template type argument.
        /// @tparam T A type satisfying BuiltinType.
        template <BuiltinType T>
        inline constexpr ValueTag value_tag_v = BuiltinTraits<T>::tag;

        /// @brief Index of T in a tuple of types (0..N-1).
        ///
        /// The primary template is intentionally undefined; the recursion below
        /// always terminates because type_index_v is gated by BuiltinType.
        /// @tparam T Type to locate.
        /// @tparam Tuple Tuple to search.
        template <typename T, typename Tuple>
        struct tuple_index;

        /// @brief Base case: T is the head of the tuple.
        template <typename T, typename... Ts>
        struct tuple_index<T, std::tuple<T, Ts...>> : std::integral_constant<size_t, 0>
        {
        };

        /// @brief Recursive case: T appears later in the tuple.
        template <typename T, typename U, typename... Ts>
        struct tuple_index<T, std::tuple<U, Ts...>>
            : std::integral_constant<size_t, 1 + tuple_index<T, std::tuple<Ts...>>::value>
        {
        };

        /// @brief Compile-time 0..N-1 index for each BuiltinType, used with
        ///        std::get<N> on the tuple storage in ParseContext.
        /// @tparam T A type satisfying BuiltinType.
        template <BuiltinType T>
        inline constexpr size_t type_index_v = tuple_index<T, BuiltinTypes>::value;

        /// @brief True when every BuiltinTypes row's tag ordinal equals its
        ///        storage index.
        /// @tparam Ts BuiltinTypes elements.
        /// @return Whether the order of ValueTag matches BuiltinTypes.
        template <typename... Ts>
        constexpr bool builtin_tags_match_order(std::tuple<Ts...> *) noexcept
        {
            return (
                (static_cast<size_t>(BuiltinTraits<Ts>::tag) == type_index_v<Ts>) && ...);
        }

        static_assert(
            builtin_tags_match_order(static_cast<BuiltinTypes *>(nullptr)),
            "ValueTag ordinal must equal the BuiltinTypes storage index");
        static_assert(
            static_cast<size_t>(ValueTag::Count) == std::tuple_size_v<BuiltinTypes>,
            "ValueTag::Count must equal the number of BuiltinTypes rows: add the "
            "BuiltinTypes row + BuiltinTraits specialization, or remove the stray "
            "ValueTag enumerator");

        /// @brief Hint labels for the BuiltinTypes rows, in storage order.
        /// @tparam Ts BuiltinTypes elements.
        /// @return Array of labels indexed by ValueTag ordinal.
        template <typename... Ts>
        constexpr std::array<std::string_view, sizeof...(Ts)> hint_names(
            std::tuple<Ts...> *)
        {
            return {BuiltinTraits<Ts>::hint_name...};
        }
    }

}  // namespace pjh::cli

#endif  // INCLUDE_PJH_CLI_TYPE_HPP
