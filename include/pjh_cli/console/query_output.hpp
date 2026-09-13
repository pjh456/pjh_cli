#ifndef INCLUDE_PJH_CLI_CONSOLE_QUERY_OUTPUT_HPP
#define INCLUDE_PJH_CLI_CONSOLE_QUERY_OUTPUT_HPP

#include <pjh_cli/console/query_result.hpp>
#include <string>
#include <string_view>

namespace pjh::cli
{

    class QueryOutput
    {
    public:
        QueryOutput() = delete;

        static std::string format(const QueryResult &result);

        static std::string format(const BranchCommand &root, std::string_view query);
    };

}  // namespace pjh::cli

#endif