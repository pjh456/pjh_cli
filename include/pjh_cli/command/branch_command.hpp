#ifndef INCLUDE_PJH_CLI_COMMAND_BRANCH_COMMAND_HPP
#define INCLUDE_PJH_CLI_COMMAND_BRANCH_COMMAND_HPP

#include <unordered_map>

#include "base_command.hpp"
#include "leaf_command.hpp"

namespace pjh::cli
{

    /// @brief Branch node in the command tree.
    ///
    /// A BranchCommand holds child subcommands (branches or leaves) but has
    /// no positional arguments of its own.  It is the only kind of command
    /// that can call add_branch() / add_leaf(), which return typed references.
    ///
    /// App inherits BranchCommand and represents the root of the tree.
    class BranchCommand : public BaseCommand
    {
    public:
        using BaseCommand::BaseCommand;
        ~BranchCommand() override = default;

        BranchCommand *as_branch() noexcept override { return this; }
        const BranchCommand *as_branch() const noexcept override { return this; }

        /// @brief Add a child branch subcommand (can itself contain subcommands).
        /// @return Reference to the newly created BranchCommand.
        BranchCommand &add_branch(std::string name, std::string description);

        /// @brief Add a child leaf subcommand (can have positional args).
        /// @return Reference to the newly created LeafCommand.
        LeafCommand &add_leaf(std::string name, std::string description);

        /// @brief Find a direct child subcommand by exact name match.
        BaseCommand *find_subcommand(std::string_view name) noexcept;

        /// @brief Const overload.
        const BaseCommand *find_subcommand(std::string_view name) const noexcept;

        /// @brief Direct child subcommands.
        std::deque<std::unique_ptr<BaseCommand>> &subcommands() noexcept
        {
            return m_subcommands;
        }

        /// @brief Const overload of subcommands().
        const std::deque<std::unique_ptr<BaseCommand>> &subcommands() const noexcept
        {
            return m_subcommands;
        }

    private:
        std::deque<std::unique_ptr<BaseCommand>> m_subcommands;
        std::unordered_map<
            std::string,
            BaseCommand *,
            detail::transparent_string_hash,
            std::equal_to<void>>
            m_subcommand_by_name;
    };

}  // namespace pjh::cli

#endif
