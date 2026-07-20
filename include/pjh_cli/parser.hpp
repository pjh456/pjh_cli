#ifndef INCLUDE_PJH_CLI_PARSER_HPP
#define INCLUDE_PJH_CLI_PARSER_HPP

#include <span>
#include <string_view>
#include <vector>

#include "command/branch_command.hpp"
#include "parse_context.hpp"
#include "type.hpp"

namespace pjh::cli
{
    /// @brief Parse args against a command tree.
    /// @param max_fuzzy_distance 0 = exact only, >0 = also try fuzzy (Levenshtein).
    CliResult<ParseContext> parse_command(
        BaseCommand &root,
        std::span<const std::string_view> args,
        int max_fuzzy_distance = 0);

    /// @brief Convenience overload: converts argv[1..argc-1] to string_views internally.
    inline CliResult<ParseContext> parse_command(
        BaseCommand &root, int argc, char **argv, int max_fuzzy_distance = 0)
    {
        std::vector<std::string_view> args;
        args.reserve(static_cast<size_t>(argc) - 1);
        for (int a = 1; a < argc; a++) args.emplace_back(argv[a]);
        return parse_command(root, args, max_fuzzy_distance);
    }
}

#endif
