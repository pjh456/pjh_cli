#include <algorithm>
#include <pjh_cli/command/base_command.hpp>
#include <pjh_cli/format/info.hpp>
#include <pjh_cli/parse/parse_context.hpp>
#include <sstream>
#include <string>
#include <vector>

namespace pjh::cli
{
    /// @brief Get the matched subcommand chain (root excluded).
    ///
    /// Walks parent pointers from the matched command to the root, reverses,
    /// and removes the root entry.
    /// @return Vector of command pointers in top-down order, empty if nothing
    ///         matched.
    std::vector<BaseCommand *> ParseContext::matched_commands()
    {
        if (!m_matched_cmd)
            return {};
        std::vector<BaseCommand *> result;
        for (auto *c = m_matched_cmd; c; c = c->parent()) result.push_back(c);
        std::reverse(result.begin(), result.end());
        if (result.size() > 1)
            result.erase(result.begin());
        return result;
    }

    /// @brief Const overload of matched_commands().
    std::vector<const BaseCommand *> ParseContext::matched_commands() const
    {
        if (!m_matched_cmd)
            return {};
        std::vector<const BaseCommand *> result;
        for (auto *c = m_matched_cmd; c; c = c->parent()) result.push_back(c);
        std::reverse(result.begin(), result.end());
        if (result.size() > 1)
            result.erase(result.begin());
        return result;
    }

    /// @brief Build a MatchedPath struct from the matched command chain.
    /// @return MatchedPath with command names in top-down order.
    MatchedPath ParseContext::matched_path_info() const
    {
        auto cmds = matched_commands();
        MatchedPath out;
        out.commands.reserve(cmds.size());
        for (auto *c : cmds)
            out.commands.push_back(c->name());
        return out;
    }

    /// @brief Full matched subcommand path as a space-separated string.
    /// @return e.g. "config set".
    std::string ParseContext::matched_path() const
    {
        auto info = matched_path_info();
        if (info.commands.empty())
            return {};
        std::ostringstream os;
        os << info.commands[0];
        for (size_t i = 1; i < info.commands.size(); ++i)
            os << ' ' << info.commands[i];
        return os.str();
    }
}  // namespace pjh::cli
