#include <algorithm>
#include <cstddef>
#include <functional>
#include <memory>
#include <pjh_cli/command/base_command.hpp>
#include <pjh_cli/command/branch_command.hpp>
#include <pjh_cli/command/leaf_command.hpp>
#include <pjh_cli/core/type.hpp>
#include <pjh_cli/option/option_def.hpp>
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
        m_extra_args_explicit = true;
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
        if (extra_args_explicit())
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
        if (extra_args_explicit())
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

}  // namespace pjh::cli
