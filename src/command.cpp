#include <algorithm>
#include <cstddef>
#include <functional>
#include <memory>
#include <pjh_cli/command/base_command.hpp>
#include <pjh_cli/command/branch_command.hpp>
#include <pjh_cli/command/leaf_command.hpp>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "pjh_cli/option_def.hpp"
#include "pjh_cli/parse_context.hpp"
#include "pjh_cli/type.hpp"

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

    // ── BranchCommand ──

    BranchCommand &BranchCommand::add_branch(std::string name, std::string description)
    {
        auto child =
            std::make_unique<BranchCommand>(std::move(name), std::move(description));
        child->set_parent(this);
        child->m_extra_args_policy = m_extra_args_policy;
        auto &ref = *child;
        m_subcommands.push_back(std::move(child));
        return ref;
    }

    LeafCommand &BranchCommand::add_leaf(std::string name, std::string description)
    {
        auto child =
            std::make_unique<LeafCommand>(std::move(name), std::move(description));
        child->set_parent(this);
        child->m_extra_args_policy = m_extra_args_policy;
        auto &ref = *child;
        m_subcommands.push_back(std::move(child));
        return ref;
    }

    BaseCommand *BranchCommand::find_subcommand(std::string_view name) noexcept
    {
        auto it = std::ranges::find(
            m_subcommands, name,
            [](const auto &ptr) -> const std::string & { return ptr->m_name; });
        return it != m_subcommands.end() ? it->get() : nullptr;
    }

    const BaseCommand *BranchCommand::find_subcommand(
        std::string_view name) const noexcept
    {
        auto it = std::ranges::find(
            m_subcommands, name,
            [](const auto &ptr) -> const std::string & { return ptr->m_name; });
        return it != m_subcommands.end() ? it->get() : nullptr;
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

    std::string ParseContext::matched_path() const
    {
        auto cmds = matched_commands();
        if (cmds.empty())
            return {};
        std::ostringstream os;
        os << cmds[0]->name();
        for (size_t i = 1; i < cmds.size(); ++i) os << ' ' << cmds[i]->name();
        return os.str();
    }

}  // namespace pjh::cli
