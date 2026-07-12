#ifndef INCLUDE_PJH_CLI_PARSER_HPP
#define INCLUDE_PJH_CLI_PARSER_HPP

#include "command.hpp"
#include "parse_context.hpp"
#include "type.hpp"

#include <string_view>
#include <vector>

namespace pjh::cli
{
    /// @brief Parse args against a command tree with exact matching.
    ParseResult<ParseContext>
    parse_command(
        const Command &root,
        const std::vector<std::string_view> &args);

    /// @brief Parse args against a command tree with fuzzy subcommand matching.
    ParseResult<ParseContext>
    parse_command_fuzzy(
        const Command &root,
        const std::vector<std::string_view> &args);
}

#endif
