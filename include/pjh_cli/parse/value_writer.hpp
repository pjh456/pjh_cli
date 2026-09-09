#ifndef INCLUDE_PJH_CLI_PARSE_VALUE_WRITER_HPP
#define INCLUDE_PJH_CLI_PARSE_VALUE_WRITER_HPP

#include <array>
#include <filesystem>
#include <pjh_cli/core/converter.hpp>
#include <pjh_cli/core/type.hpp>
#include <pjh_cli/parse/parse_context.hpp>
#include <pjh_cli/parse/parse_context_writer.hpp>
#include <string_view>
#include <tuple>

namespace pjh::cli
{
    /// @brief Utility for converting raw string tokens to typed values and
    ///        writing them into a ParseContext.
    ///
    /// The main entry point is apply_arg_value(), which dispatches by ValueTag
    /// to the correct template instantiation.  The per-type conversion is
    /// handled by the private convert_and_set<T>() template.
    ///
    /// Usage:
    /// @code
    ///   ValueWriter::apply_arg_value(ctx, hash, ValueTag::Int, "42");
    /// @endcode
    class ValueWriter
    {
    public:
        ValueWriter() = delete;

        /// @brief Convert a raw string to the type indicated by @p tag and
        ///        store it in @p ctx under @p hash.
        ///
        /// Indexes the generated per-tag converter table (ValueTag →
        /// convert_and_set<T>), then delegates to the selected entry.
        ///
        /// @param ctx   Parse context to write into.
        /// @param hash  Key hash identifying the value slot.
        /// @param tag   Runtime type tag (Bool / Int / Double / String / Path).
        /// @param s     Raw input string from the command line.
        /// @param display Positional arg name used in conversion errors
        ///        (e.g. "file"); empty renders `for ''`.
        /// @return Ok on success, or Err with a type-conversion error.
        static CliResult<void> apply_arg_value(
            ParseContext &ctx,
            size_t hash,
            ValueTag tag,
            std::string_view s,
            std::string_view display = {});

    private:
        /// @brief Signature of one tag→converter dispatch entry.
        using ConvertFn = CliResult<void> (*)(
            ParseContext &, size_t, std::string_view, std::string_view);

        /// @brief Build the tag→converter table from a tuple of builtin types.
        ///
        /// Entry i is convert_and_set<Ts_i>(), so the table is ordered exactly
        /// like detail::BuiltinTypes (and therefore like ValueTag).
        /// @tparam Ts BuiltinTypes elements.
        /// @return Array of converter function pointers, in storage order.
        template <typename... Ts>
        static constexpr std::array<ConvertFn, sizeof...(Ts)> make_table(
            std::tuple<Ts...> *)
        {
            return {&convert_and_set<Ts>...};
        }

        /// @brief Convert @p s to type T and store it in @p ctx.
        ///
        /// Conversion goes through Converter<T>::from_string() for every
        /// builtin type, so the call shape is uniform.
        ///
        /// @tparam T Target type (must satisfy BuiltinType).
        /// @param ctx   Parse context to write into.
        /// @param hash  Key hash identifying the value slot.
        /// @param s     Raw input string.
        /// @param display Positional arg name used in conversion errors.
        /// @return Ok on success, or Err with a type-conversion error.
        template <detail::BuiltinType T>
        static CliResult<void> convert_and_set(
            ParseContext &ctx,
            size_t hash,
            std::string_view s,
            std::string_view display = {})
        {
            auto r = Converter<T>::from_string(s, display);
            if (r.is_err())
                return CliResult<void>::Err(std::move(r).unwrap_err());
            ParseContextWriter::set_value<T>(ctx, hash, std::move(r).unwrap());
            return CliResult<void>::Ok();
        }
    };
}  // namespace pjh::cli

#endif
