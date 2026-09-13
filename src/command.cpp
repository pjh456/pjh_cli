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
        m_subcommands.push_back(std::move(child));  // 1) own first
        try
        {
            m_subcommand_by_name[ref.name()] = &ref;  // 2) index the owned child
        }
        catch (...)
        {
            // 3) roll back: release the child before rethrowing.
            m_subcommands.pop_back();
            throw;
        }
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
        m_subcommands.push_back(std::move(child));  // 1) own first
        try
        {
            m_subcommand_by_name[ref.name()] = &ref;  // 2) index the owned child
        }
        catch (...)
        {
            // 3) roll back: release the child before rethrowing.
            m_subcommands.pop_back();
            throw;
        }
        return ref;
    }

    BranchCommand::~BranchCommand()
    {
        // Iterative post-order teardown with O(1) extra space.  A node's
        // children stay in its deque while we descend into the front one, so
        // the parent link is the way back up; leaves are freed in place and a
        // drained branch is popped (and destroyed) from its parent.  Children
        // therefore die before their parent and siblings in registration order,
        // matching the default recursive teardown, with no per-level stack
        // frame and no allocation (allocation here would terminate on the OOM
        // unwind path).
        BranchCommand *cur = this;
        while (true)
        {
            while (!cur->m_subcommands.empty())
            {
                BaseCommand *child = cur->m_subcommands.front().get();
                if (auto *branch = child->as_branch())
                {
                    cur = branch;  // descend; the child stays owned by its parent
                    continue;
                }
                cur->m_subcommands.pop_front();  // leaf: free in place
            }

            if (cur == this)
                break;

            // `cur` is drained and still the front child of its branch parent.
            auto *parent = cur->m_parent->as_branch();
            parent->m_subcommands.pop_front();  // destroys the drained `cur`
            cur = parent;
        }
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
