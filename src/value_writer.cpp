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
    /// Indexes the generated per-tag converter table, whose entries are built
    /// from detail::BuiltinTypes.
    CliResult<void> ValueWriter::apply_arg_value(
        ParseContext &ctx,
        size_t hash,
        ValueTag tag,
        std::string_view s,
        std::string_view display)
    {
        static constexpr auto table =
            make_table(static_cast<detail::BuiltinTypes *>(nullptr));
        auto idx = static_cast<size_t>(tag);
        if (idx >= table.size())
            return CliResult<void>::Ok();
        return table[idx](ctx, hash, s, display);
    }
}  // namespace pjh::cli
