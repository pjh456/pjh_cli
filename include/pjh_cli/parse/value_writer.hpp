#ifndef INCLUDE_PJH_CLI_PARSE_VALUE_WRITER_HPP
#define INCLUDE_PJH_CLI_PARSE_VALUE_WRITER_HPP

#include <filesystem>
#include <pjh_cli/core/converter.hpp>
#include <pjh_cli/core/type.hpp>
#include <pjh_cli/parse/parse_context.hpp>
#include <string_view>

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
        /// Acts as a switch (ValueTag → concrete C++ type), then delegates
        /// to convert_and_set<T>().
        ///
        /// @param ctx   Parse context to write into.
        /// @param hash  Key hash identifying the value slot.
        /// @param tag   Runtime type tag (Bool / Int / Double / String / Path).
        /// @param s     Raw input string from the command line.
        /// @return Ok on success, or Err with a type-conversion error.
        static CliResult<void> apply_arg_value(
            ParseContext &ctx, size_t hash, ValueTag tag, std::string_view s);

    private:
        /// @brief Convert @p s to type T and store it in @p ctx.
        ///
        /// For std::string and std::filesystem::path the value is constructed
        /// directly.  For bool, int, and double the conversion goes through
        /// Converter<T>::from_string() which uses std::from_chars.
        ///
        /// @tparam T Target type (must satisfy BuiltinType).
        /// @param ctx   Parse context to write into.
        /// @param hash  Key hash identifying the value slot.
        /// @param s     Raw input string.
        /// @return Ok on success, or Err with a type-conversion error.
        template <detail::BuiltinType T>
        static CliResult<void> convert_and_set(
            ParseContext &ctx, size_t hash, std::string_view s)
        {
            if constexpr (std::same_as<T, std::string>)
            {
                ctx.set_value<std::string>(hash, std::string(s));
            }
            else if constexpr (std::same_as<T, std::filesystem::path>)
            {
                ctx.set_value<std::filesystem::path>(hash, std::filesystem::path(s));
            }
            else
            {
                auto r = Converter<T>::from_string(s);
                if (r.is_err())
                    return CliResult<void>::Err(std::move(r).unwrap_err());
                ctx.set_value<T>(hash, r.unwrap());
            }
            return CliResult<void>::Ok();
        }
    };
}  // namespace pjh::cli

#endif
