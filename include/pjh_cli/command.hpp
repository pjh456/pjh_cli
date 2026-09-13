#ifndef INCLUDE_PJH_CLI_COMMAND_HPP
#define INCLUDE_PJH_CLI_COMMAND_HPP

/// @brief Umbrella header for the command tree.
///
/// Re-exports the composite command model: BaseCommand (node contract),
/// BranchCommand (child subcommands), LeafCommand (positional args), and
/// command_builder.hpp (the .option()/.group() builder hub).  App, the root
/// command, lives in <pjh_cli/app.hpp>.

#include <pjh_cli/command/base_command.hpp>
#include <pjh_cli/command/branch_command.hpp>
#include <pjh_cli/command/command_builder.hpp>
#include <pjh_cli/command/leaf_command.hpp>

#endif  // INCLUDE_PJH_CLI_COMMAND_HPP
