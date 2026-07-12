#include <pjh_cli/command.hpp>

namespace pjh::cli
{
    Command::Command(
        std::string name,
        std::string description)
        : m_name(std::move(name)),
          m_description(std::move(description))
    {
    }

    Command &
    Command::add_command(
        std::string name,
        std::string description)
    {
        m_subcommands.emplace_back(
            std::move(name),
            std::move(description));
        auto &child = m_subcommands.back();
        child.m_parent = this;
        return child;
    }

    Command *
    Command::find_subcommand(
        std::string_view name)
    {
        for (auto &cmd : m_subcommands)
            if (cmd.m_name == name)
                return &cmd;
        return nullptr;
    }

    const Command *
    Command::find_subcommand(
        std::string_view name) const
    {
        for (const auto &cmd : m_subcommands)
            if (cmd.m_name == name)
                return &cmd;
        return nullptr;
    }

    const OptionDef *
    Command::find_option_by_long(
        std::string_view name) const
    {
        auto it = m_option_by_long.find(std::string(name));
        if (it == m_option_by_long.end())
            return nullptr;
        return &m_options[it->second];
    }

    const OptionDef *
    Command::find_option_by_short(char c) const
    {
        auto it = m_option_by_short.find(c);
        if (it == m_option_by_short.end())
            return nullptr;
        return &m_options[it->second];
    }

    ParseContext
    Command::create_context() const
    {
        return ParseContext{};
    }

    ParseResult<void>
    Command::apply_defaults(
        ParseContext &ctx) const
    {
        for (auto &opt : m_options)
            if (opt.m_apply_default &&
                !ctx.has_value(opt.m_key_hash))
                opt.m_apply_default(ctx);
        return ParseResult<void>::Ok();
    }

    ParseResult<void>
    Command::execute(ParseContext &ctx) const
    {
        if (!m_action)
            return ParseResult<void>::Ok();
        return m_action(ctx);
    }

    Command &
    Command::action(
        std::function<
            ParseResult<void>(ParseContext &)>
            fn)
    {
        m_action = std::move(fn);
        return *this;
    }

    Command &
    Command::enabled(
        std::function<bool()> pred)
    {
        m_enabled = std::move(pred);
        return *this;
    }

    Command &
    Command::set_visibility(Visibility v)
    {
        m_visibility = v;
        return *this;
    }

} // namespace pjh::cli
