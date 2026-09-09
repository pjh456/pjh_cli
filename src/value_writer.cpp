#include <filesystem>
#include <pjh_cli/core/converter.hpp>
#include <pjh_cli/core/type.hpp>
#include <pjh_cli/parse/parse_context.hpp>
#include <pjh_cli/parse/value_writer.hpp>
#include <utility>
#include <variant>

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

    /// @brief Store an already-converted option value, appending when
    ///        repeatable.
    void ValueWriter::apply_option_value(
        ParseContext &ctx, size_t hash, bool repeatable, OptionValue value)
    {
        std::visit(
            [&](auto &&typed)
            {
                using T = std::decay_t<decltype(typed)>;
                if (repeatable)
                    detail::ParseContextWriter::append_value<T>(
                        ctx, hash, std::forward<decltype(typed)>(typed));
                else
                    detail::ParseContextWriter::set_value<T>(
                        ctx, hash, std::forward<decltype(typed)>(typed));
            },
            std::move(value));
    }

    /// @brief Run @p opt's convert+validate pipeline on @p raw and store the
    ///        result.
    CliResult<void> ValueWriter::apply_option_raw(
        ParseContext &ctx, const OptionDef &opt, std::string_view raw)
    {
        auto r = opt.parse_value(raw);
        if (r.is_err())
            return CliResult<void>::Err(std::move(r).unwrap_err());
        apply_option_value(
            ctx, opt.key_hash(), opt.is_repeatable(), std::move(r).unwrap());
        return CliResult<void>::Ok();
    }
}  // namespace pjh::cli
