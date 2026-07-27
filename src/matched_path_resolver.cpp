#include <algorithm>
#include <pjh_cli/command/base_command.hpp>
#include <pjh_cli/format/info.hpp>
#include <pjh_cli/parse/matched_path_resolver.hpp>
#include <sstream>
#include <string>
#include <vector>

namespace pjh::cli
{
    /// @brief Walk the parent chain from @p matched_cmd to the root,
    ///        collecting every command along the way, then remove the root
    ///        and reverse to get top-down order.
    std::vector<const BaseCommand *> MatchedPathResolver::resolve_chain(
        const BaseCommand *matched_cmd)
    {
        if (!matched_cmd)
            return {};
        std::vector<const BaseCommand *> result;
        for (auto *c = matched_cmd; c; c = c->parent()) result.push_back(c);
        std::reverse(result.begin(), result.end());
        if (result.size() > 1)
            result.erase(result.begin());
        return result;
    }

    /// @brief Build a MatchedPath struct by resolving the chain and
    ///        collecting name() from each command.
    MatchedPath MatchedPathResolver::to_path_info(
        const BaseCommand *matched_cmd)
    {
        auto cmds = resolve_chain(matched_cmd);
        MatchedPath out;
        out.commands.reserve(cmds.size());
        for (auto *c : cmds)
            out.commands.push_back(c->name());
        return out;
    }

    /// @brief Format the matched path as a space-separated string.
    ///
    /// Delegates to to_path_info() and joins the command names.
    std::string MatchedPathResolver::to_path_string(
        const BaseCommand *matched_cmd)
    {
        auto info = to_path_info(matched_cmd);
        if (info.commands.empty())
            return {};
        std::ostringstream os;
        os << info.commands[0];
        for (size_t i = 1; i < info.commands.size(); ++i)
            os << ' ' << info.commands[i];
        return os.str();
    }
}  // namespace pjh::cli
