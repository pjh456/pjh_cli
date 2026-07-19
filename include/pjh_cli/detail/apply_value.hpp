#ifndef INCLUDE_PJH_CLI_DETAIL_APPLY_VALUE_HPP
#define INCLUDE_PJH_CLI_DETAIL_APPLY_VALUE_HPP

#include "../converter.hpp"
#include "../parse_context.hpp"
#include "../type.hpp"

namespace pjh::cli::detail
{
    inline CliResult<void> apply_value(
        ParseContext &ctx, size_t hash, ValueTag tag, std::string_view s)
    {
        switch (tag)
        {
        case ValueTag::Double:
        {
            auto r = Converter<double>::from_string(s);
            if (r.is_err())
                return CliResult<void>::Err(std::move(r).unwrap_err());
            ctx.set_value<double>(hash, r.unwrap());
            return CliResult<void>::Ok();
        }
        case ValueTag::String:
        {
            auto r = Converter<std::string>::from_string(s);
            if (r.is_err())
                return CliResult<void>::Err(std::move(r).unwrap_err());
            ctx.set_value<std::string>(hash, r.unwrap());
            return CliResult<void>::Ok();
        }
        case ValueTag::Path:
            ctx.set_value<std::filesystem::path>(hash, std::filesystem::path(s));
            return CliResult<void>::Ok();
        default:
        {
            auto r = Converter<int>::from_string(s);
            if (r.is_err())
                return CliResult<void>::Err(std::move(r).unwrap_err());
            ctx.set_value<int>(hash, r.unwrap());
            return CliResult<void>::Ok();
        }
        }
    }
}

#endif
