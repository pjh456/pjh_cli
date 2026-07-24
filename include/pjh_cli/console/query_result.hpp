#ifndef INCLUDE_PJH_CLI_CONSOLE_QUERY_RESULT_HPP
#define INCLUDE_PJH_CLI_CONSOLE_QUERY_RESULT_HPP

#include <pjh_cli/format/info.hpp>
#include <string>
#include <vector>

namespace pjh::cli
{

    enum class QueryKind
    {
        Listing,       ///< Empty query — show all subcommands
        Matched,       ///< Substring match found — show matched names
        Fuzzy,         ///< No substring match, fuzzy suggestions available
        NoMatch,       ///< No substring or fuzzy match
    };

    struct QueryResult
    {
        QueryKind kind = QueryKind::Listing;

        /// Names for Listing / Matched kinds.
        std::vector<std::string> names;

        /// Fuzzy suggestions for Fuzzy kind.
        SuggestionInfo suggestions;

        /// Usage hint for NoMatch kind.
        std::string usage_line;
    };

}  // namespace pjh::cli

#endif