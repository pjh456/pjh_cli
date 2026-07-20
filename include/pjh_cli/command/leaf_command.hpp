#ifndef INCLUDE_PJH_CLI_COMMAND_LEAF_COMMAND_HPP
#define INCLUDE_PJH_CLI_COMMAND_LEAF_COMMAND_HPP

#include "../arg_def.hpp"
#include "base_command.hpp"

namespace pjh::cli
{

    class LeafCommand : public BaseCommand
    {
    public:
        using BaseCommand::BaseCommand;
        ~LeafCommand() override = default;

        LeafCommand *as_leaf() noexcept override { return this; }
        const LeafCommand *as_leaf() const noexcept override { return this; }

        template <typename T, size_t Index>
            requires detail::BuiltinType<T>
        ArgDef &arg(std::string name, std::string description)
        {
            constexpr size_t h = key_hash(Index);

            auto &def = m_args.emplace_back();
            def.m_name = std::move(name);
            def.m_description = std::move(description);
            def.m_index = Index;
            def.m_key_hash = h;
            def.m_value_tag = detail::value_tag_v<T>;

            return def;
        }

        const std::deque<ArgDef> &args() const noexcept { return m_args; }

    private:
        std::deque<ArgDef> m_args;
    };

}  // namespace pjh::cli

#endif
