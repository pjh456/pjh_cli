#include <filesystem>
#include <pjh_cli/core/converter.hpp>
#include <pjh_cli/core/type.hpp>
#include <pjh_cli/parse/parse_context.hpp>
#include <pjh_cli/parse/value_writer.hpp>

namespace pjh::cli
{
    /// @brief Convert a raw string to the type indicated by @p tag and
    ///        store it in @p ctx under @p hash.
    ///
    /// Dispatches by ValueTag to the correct convert_and_set<T>() instantiation.
    CliResult<void> ValueWriter::apply_arg_value(
        ParseContext &ctx, size_t hash, ValueTag tag, std::string_view s)
    {
        switch (tag)
        {
        case ValueTag::Bool:
            return convert_and_set<bool>(ctx, hash, s);
        case ValueTag::Int:
            return convert_and_set<int>(ctx, hash, s);
        case ValueTag::Double:
            return convert_and_set<double>(ctx, hash, s);
        case ValueTag::String:
            return convert_and_set<std::string>(ctx, hash, s);
        case ValueTag::Path:
            return convert_and_set<std::filesystem::path>(ctx, hash, s);
        }
        return CliResult<void>::Ok();
    }
}  // namespace pjh::cli
