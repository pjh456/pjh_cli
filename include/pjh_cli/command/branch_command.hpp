#ifndef INCLUDE_PJH_CLI_COMMAND_BRANCH_COMMAND_HPP
#define INCLUDE_PJH_CLI_COMMAND_BRANCH_COMMAND_HPP

#include "base_command.hpp"

namespace pjh::cli
{

    class LeafCommand;

    class BranchCommand : public BaseCommand
    {
    public:
        using BaseCommand::BaseCommand;
        ~BranchCommand() override = default;

        BranchCommand *as_branch() noexcept override { return this; }
        const BranchCommand *as_branch() const noexcept override { return this; }

        BranchCommand &add_branch(std::string name, std::string description);
        LeafCommand &add_leaf(std::string name, std::string description);

        BaseCommand *find_subcommand(std::string_view name) noexcept;
        const BaseCommand *find_subcommand(std::string_view name) const noexcept;

        std::deque<std::unique_ptr<BaseCommand>> &subcommands() noexcept
        {
            return m_subcommands;
        }

        const std::deque<std::unique_ptr<BaseCommand>> &subcommands() const noexcept
        {
            return m_subcommands;
        }

    private:
        std::deque<std::unique_ptr<BaseCommand>> m_subcommands;
    };

}  // namespace pjh::cli

#endif
