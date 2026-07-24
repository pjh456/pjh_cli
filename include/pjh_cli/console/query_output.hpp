#ifndef INCLUDE_PJH_CLI_CONSOLE_QUERY_OUTPUT_HPP
#define INCLUDE_PJH_CLI_CONSOLE_QUERY_OUTPUT_HPP

#include <pjh_cli/console/query_result.hpp>
#include <functional>
#include <string>

namespace pjh::cli
{

    using QueryFormatter = std::function<std::string(const QueryResult &)>;

    class QueryOutput
    {
    public:
        QueryOutput() = delete;

        static std::string format(const QueryResult &result);

        static std::string format(
            const BranchCommand &root,
            std::string_view query);

        static std::string format(
            const BranchCommand &root,
            std::string_view query,
            const QueryFormatter &custom_fmt);
    };

}  // namespace pjh::cli

#endif