#ifndef INCLUDE_PJH_CLI_PARSE_VALUE_WRITER_HPP
#define INCLUDE_PJH_CLI_PARSE_VALUE_WRITER_HPP

#include <array>
#include <cstddef>
#include <filesystem>
#include <pjh_cli/core/converter.hpp>
#include <pjh_cli/core/type.hpp>
#include <pjh_cli/option/option_def.hpp>
#include <pjh_cli/parse/detail/parse_context_writer.hpp>
#include <pjh_cli/parse/parse_context.hpp>
#include <string_view>
#include <tuple>
#include <utility>

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
        ///              Must be a real tag; ValueTag::Count and any cast from an
        ///              out-of-range integer are rejected.
        /// @param s     Raw input string from the command line.
        /// @param display Positional arg name used in conversion errors
        ///        (e.g. "file"); empty renders `for ''`.
        /// @return Ok on success, or Err with a type-conversion error.
        /// @throws LogicError if @p tag is not a real ValueTag (enum/table
        ///         mismatch or an invalid cast) — an internal invariant
        ///         violation, not a user parse error.
        static CliResult<void> apply_arg_value(
            ParseContext &ctx,
            size_t hash,
            ValueTag tag,
            std::string_view s,
            std::string_view display = {});

        /// @brief Store an already-converted option value, appending when
        ///        repeatable.
        ///
        /// The std::visit dispatch is exhaustive by construction: the
        /// OptionValue alternative set is derived from detail::BuiltinTypes
        /// and every alternative moves noexcept, so valueless_by_exception()
        /// is unreachable.
        ///
        /// @param ctx        Parse context to write into.
        /// @param hash       Option key hash (OptionDef::key_hash()).
        /// @param repeatable Whether to append (OptionDef::is_repeatable()).
        /// @param value      Converted value from OptionDef::parse_value().
        /// @param origin     Where the value came from; only
        ///        ValueOrigin::CommandLine records the key as explicitly
        ///        provided for ParseContext::was_provided().
        static void apply_option_value(
            ParseContext &ctx,
            size_t hash,
            bool repeatable,
            OptionValue value,
            ValueOrigin origin = ValueOrigin::CommandLine);

        /// @brief Run @p opt's convert+validate pipeline on @p raw and store
        ///        the result.
        /// @param ctx Parse context to write into.
        /// @param opt Option whose pipeline to run.
        /// @param raw   Raw string value.
        /// @param origin Where the value came from (forwarded to
        ///        apply_option_value).
        /// @return Ok, or the conversion/validation error verbatim.
        static CliResult<void> apply_option_raw(
            ParseContext &ctx,
            const OptionDef &opt,
            std::string_view raw,
            ValueOrigin origin = ValueOrigin::CommandLine);

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
            detail::ParseContextWriter::set_value<T>(ctx, hash, std::move(r).unwrap());
            return CliResult<void>::Ok();
        }
    };
}  // namespace pjh::cli

#endif
