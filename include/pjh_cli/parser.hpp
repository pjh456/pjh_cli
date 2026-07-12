#ifndef INCLUDE_PJH_CLI_PARSER_HPP
#define INCLUDE_PJH_CLI_PARSER_HPP

#include "command.hpp"
#include "parse_context.hpp"
#include "type.hpp"

#include <span>
#include <string_view>

namespace pjh::cli
{
    /// @brief Parse args against a command tree.
    /// @param max_fuzzy_distance 0 = exact only, >0 = also try fuzzy (Levenshtein).
    ParseResult<ParseContext>
    parse_command(
        const Command &root,
        std::span<const std::string_view> args,
        int max_fuzzy_distance = 0);
}

#endif
