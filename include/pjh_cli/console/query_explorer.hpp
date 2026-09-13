#ifndef INCLUDE_PJH_CLI_CONSOLE_QUERY_EXPLORER_HPP
#define INCLUDE_PJH_CLI_CONSOLE_QUERY_EXPLORER_HPP

#include <pjh_cli/command/branch_command.hpp>
#include <pjh_cli/console/query_result.hpp>
#include <string_view>

namespace pjh::cli
{

    class QueryExplorer
    {
    public:
        QueryExplorer() = delete;

        static QueryResult explore(const BranchCommand &root, std::string_view query);
    };

}  // namespace pjh::cli

#endif