#ifndef INCLUDE_PJH_CLI_COMMAND_LEAF_COMMAND_HPP
#define INCLUDE_PJH_CLI_COMMAND_LEAF_COMMAND_HPP

#include <pjh_cli/command/base_command.hpp>
#include <pjh_cli/option/arg_def.hpp>
#include <string>

namespace pjh::cli
{

    /// @brief Leaf node in the command tree.
    ///
    /// A LeafCommand holds positional arguments (arg<T, Index>()) and has
    /// no subcommands.  It is the terminal node used for actions that take
    /// typed positional inputs.  Positional arguments are registered
    /// strictly in index order (0, 1, 2, ...), so registration order,
    /// compile-time Index, parser positional slot and stored key hash stay
    /// identical.
    class LeafCommand : public BaseCommand
    {
    public:
        using BaseCommand::BaseCommand;
        ~LeafCommand() override = default;

        LeafCommand *as_leaf() noexcept override { return this; }
        const LeafCommand *as_leaf() const noexcept override { return this; }

        /// @brief Register a positional argument identified by compile-time Index.
        ///
        /// Positional arguments are registered strictly in ascending index
        /// order starting at 0: @p Index must equal the current number of
        /// registered arguments (`args().size()`).  This keeps registration
        /// order, the compile-time Index, the parser's positional slot and
        /// the stored key hash aligned (`args()[i].m_key_hash == i`).
        /// @tparam T Value type.
        /// @tparam Index Positional index; must be the next contiguous
        /// position (0, 1, 2, ...).
        /// @param name Display name for help / error messages.
        /// @param description Help text description.
        /// @return Reference to the registered definition (stable across
        /// later registrations).
        /// @throws LogicError If @p Index is not the next contiguous position
        /// (duplicate, skipped, out-of-order or oversized index).
        template <typename T, size_t Index>
            requires detail::BuiltinType<T>
        ArgDef &arg(std::string name, std::string description)
        {
            constexpr size_t h = key_hash(Index);

            if (Index != m_args.size())
                throw LogicError(
                    "LeafCommand::arg: positional index " + std::to_string(Index) +
                    " on command '" + this->name() + "' is out of sequence (expected " +
                    std::to_string(m_args.size()) + ")");

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
