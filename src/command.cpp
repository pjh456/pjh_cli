#include <algorithm>
#include <cstddef>
#include <functional>
#include <memory>
#include <pjh_cli/command/base_command.hpp>
#include <pjh_cli/command/branch_command.hpp>
#include <pjh_cli/command/leaf_command.hpp>
#include <pjh_cli/core/type.hpp>
#include <pjh_cli/option/option_def.hpp>
#include <pjh_cli/parse/parse_context.hpp>
#include <pjh_cli/format/info.hpp>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace pjh::cli
{

    // ── BaseCommand ──

    BaseCommand::BaseCommand(std::string name, std::string description) :
        m_name(std::move(name)), m_description(std::move(description))
    {
    }

    const OptionDef *BaseCommand::find_option_by_long(
        std::string_view name) const noexcept
    {
        auto it = m_option_by_long.find(name);
        if (it == m_option_by_long.end())
            return nullptr;
        return it->second;
    }

    const OptionDef *BaseCommand::find_option_by_short(char c) const noexcept
    {
        auto it = m_option_by_short.find(c);
        if (it == m_option_by_short.end())
            return nullptr;
        return it->second;
    }

    ParseContext BaseCommand::create_context() const noexcept { return ParseContext{}; }

    CliResult<void> BaseCommand::apply_defaults(ParseContext &ctx) const
    {
        for (const auto &opt_ptr : m_options)
        {
            if (!opt_ptr->has_default() || ctx.has_value(opt_ptr->key_hash()))
                continue;
            auto r = opt_ptr->apply_default(ctx);
            if (r.is_err())
                return r;
        }
        return CliResult<void>::Ok();
    }

    CliResult<void> BaseCommand::execute(ParseContext &ctx) const
    {
        if (!m_action)
            return CliResult<void>::Ok();
        return m_action(ctx);
    }

    BaseCommand &BaseCommand::action(std::function<CliResult<void>(ParseContext &)> fn)
    {
        m_action = std::move(fn);
        return *this;
    }

    BaseCommand &BaseCommand::enabled(std::function<bool()> pred)
    {
        m_enabled = std::move(pred);
        return *this;
    }

    BaseCommand &BaseCommand::set_visibility(Visibility v)
    {
        m_visibility = v;
        return *this;
    }

    BaseCommand &BaseCommand::set_extra_args(ExtraArgsPolicy p)
    {
        m_extra_args_policy = p;
        return *this;
    }

    BaseCommand &BaseCommand::alias(std::string name)
    {
        m_aliases.push_back(std::move(name));
        return *this;
    }

    // ── BranchCommand ──

    BranchCommand &BranchCommand::add_branch(std::string name, std::string description)
    {
        auto child =
            std::make_unique<BranchCommand>(std::move(name), std::move(description));
        child->set_parent(this);
        child->set_extra_args(extra_args_policy());
        child->set_visibility(visibility());
        child->enabled(m_enabled);
        auto &ref = *child;
        m_subcommand_by_name[ref.name()] = &ref;
        m_subcommands.push_back(std::move(child));
        return ref;
    }

    LeafCommand &BranchCommand::add_leaf(std::string name, std::string description)
    {
        auto child =
            std::make_unique<LeafCommand>(std::move(name), std::move(description));
        child->set_parent(this);
        child->set_extra_args(extra_args_policy());
        child->set_visibility(visibility());
        child->enabled(m_enabled);
        auto &ref = *child;
        m_subcommand_by_name[ref.name()] = &ref;
        m_subcommands.push_back(std::move(child));
        return ref;
    }

    BaseCommand *BranchCommand::find_subcommand(std::string_view name) noexcept
    {
        auto it = m_subcommand_by_name.find(name);
        if (it != m_subcommand_by_name.end())
            return it->second;
        for (auto &sub : m_subcommands)
            for (auto &a : sub->aliases())
                if (a == name)
                    return sub.get();
        return nullptr;
    }

    const BaseCommand *BranchCommand::find_subcommand(
        std::string_view name) const noexcept
    {
        auto it = m_subcommand_by_name.find(name);
        if (it != m_subcommand_by_name.end())
            return it->second;
        for (const auto &sub : m_subcommands)
            for (const auto &a : sub->aliases())
                if (a == name)
                    return sub.get();
        return nullptr;
    }

    // ── ParseContext ──

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

    MatchedPath ParseContext::matched_path_info() const
    {
        auto cmds = matched_commands();
        MatchedPath out;
        out.commands.reserve(cmds.size());
        for (auto *c : cmds)
            out.commands.push_back(c->name());
        return out;
    }

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
