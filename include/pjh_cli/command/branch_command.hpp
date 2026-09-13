#ifndef INCLUDE_PJH_CLI_COMMAND_BRANCH_COMMAND_HPP
#define INCLUDE_PJH_CLI_COMMAND_BRANCH_COMMAND_HPP

#include <pjh_cli/command/base_command.hpp>
#include <pjh_cli/command/leaf_command.hpp>
#include <unordered_map>

namespace pjh::cli
{

    /// @brief Branch node in the command tree.
    ///
    /// A BranchCommand holds child subcommands (branches or leaves) but has
    /// no positional arguments of its own.  It is the only kind of command
    /// that can call add_branch() / add_leaf(), which return typed references.
    /// The child list is exposed read-only; mutation happens only through
    /// add_branch() / add_leaf().
    ///
    /// App inherits BranchCommand and represents the root of the tree.
    class BranchCommand : public BaseCommand
    {
    public:
        using BaseCommand::BaseCommand;

        /// @brief Destroy the branch, tearing down the child subtree iteratively.
        ///
        /// Children are destroyed by walking the existing parent links rather
        /// than one recursive stack frame per nesting level, so an arbitrarily
        /// deep tree cannot overflow the stack.  Ordering matches the default
        /// recursive teardown: each child subtree before its parent, siblings in
        /// registration order.  The walk uses O(1) extra space and allocates
        /// nothing, so it also stays safe during exception unwinding.
        ~BranchCommand() override;

        BranchCommand *as_branch() noexcept override { return this; }
        const BranchCommand *as_branch() const noexcept override { return this; }

        /// @brief Add a child branch subcommand (can itself contain subcommands).
        /// @note Strong exception guarantee: if indexing throws, the child is
        ///       removed and the branch is unchanged (no dangling lookup entry).
        /// @return Reference to the newly created BranchCommand.
        BranchCommand &add_branch(std::string name, std::string description);

        /// @brief Add a child leaf subcommand (can have positional args).
        /// @note Strong exception guarantee: if indexing throws, the child is
        ///       removed and the branch is unchanged (no dangling lookup entry).
        /// @return Reference to the newly created LeafCommand.
        LeafCommand &add_leaf(std::string name, std::string description);

        /// @brief Find a direct child subcommand by exact name match.
        BaseCommand *find_subcommand(std::string_view name) noexcept;

        /// @brief Const overload.
        const BaseCommand *find_subcommand(std::string_view name) const noexcept;

        /// @brief Read-only view of the direct child subcommands.
        ///
        /// Children are created exclusively through add_branch() / add_leaf(),
        /// which set the parent link and the name index.  The returned reference
        /// is read-only: inserting, erasing, or reordering through it would
        /// bypass that bookkeeping and orphan children (or leave dangling index
        /// entries).  Iteration order is registration order.
        ///
        /// Note: element access yields a non-const BaseCommand* (the container
        /// is read-only, the pointee is not); child configuration still goes
        /// through BaseCommand's public setters.
        ///
        /// @return Const reference to the owned child list.
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
