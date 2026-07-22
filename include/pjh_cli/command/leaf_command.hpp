#ifndef INCLUDE_PJH_CLI_COMMAND_LEAF_COMMAND_HPP
#define INCLUDE_PJH_CLI_COMMAND_LEAF_COMMAND_HPP

#include <pjh_cli/arg_def.hpp>
#include <pjh_cli/command/base_command.hpp>

namespace pjh::cli
{

    /// @brief Leaf node in the command tree.
    ///
    /// A LeafCommand holds positional arguments (arg<T, Index>()) and has
    /// no subcommands.  It is the terminal node used for actions that take
    /// typed positional inputs.
    class LeafCommand : public BaseCommand
    {
    public:
        using BaseCommand::BaseCommand;
        ~LeafCommand() override = default;

        LeafCommand *as_leaf() noexcept override { return this; }
        const LeafCommand *as_leaf() const noexcept override { return this; }

        /// @brief Register a positional argument identified by compile-time Index.
        /// @tparam T Value type.
        /// @tparam Index Positional index (0, 1, 2, ...).
        template <typename T, size_t Index>
            requires detail::BuiltinType<T>
        ArgDef &arg(std::string name, std::string description)
        {
            constexpr size_t h = key_hash(Index);

            auto &def = m_args.emplace_back();
            def.m_name = std::move(name);
            def.m_description = std::move(description);
            def.m_key_hash = h;
            def.m_value_tag = detail::value_tag_v<T>;

            return def;
        }

        /// @brief Registered positional arguments.
        const std::deque<ArgDef> &args() const noexcept { return m_args; }

    private:
        std::deque<ArgDef> m_args;
    };

}  // namespace pjh::cli

#endif
