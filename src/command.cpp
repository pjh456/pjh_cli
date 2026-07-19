#include <algorithm>
#include <pjh_cli/command.hpp>
#include <pjh_cli/detail/apply_value.hpp>

namespace pjh::cli
{
    Command::Command(std::string name, std::string description) :
        m_name(std::move(name)), m_description(std::move(description))
    {
    }

    Command &Command::add_command(std::string name, std::string description)
    {
        m_subcommands.emplace_back(std::move(name), std::move(description));
        auto &child = m_subcommands.back();
        child.m_parent = this;
        child.m_extra_args_policy = m_extra_args_policy;
        return child;
    }

    Command *Command::find_subcommand(std::string_view name) noexcept
    {
        auto it = std::ranges::find(m_subcommands, name, &Command::m_name);
        return it != m_subcommands.end() ? &*it : nullptr;
    }

    const Command *Command::find_subcommand(std::string_view name) const noexcept
    {
        auto it = std::ranges::find(m_subcommands, name, &Command::m_name);
        return it != m_subcommands.end() ? &*it : nullptr;
    }

    const OptionDef *Command::find_option_by_long(std::string_view name) const noexcept
    {
        auto it = m_option_by_long.find(name);
        if (it == m_option_by_long.end())
            return nullptr;
        return &m_options[it->second];
    }

    const OptionDef *Command::find_option_by_short(char c) const noexcept
    {
        auto it = m_option_by_short.find(c);
        if (it == m_option_by_short.end())
            return nullptr;
        return &m_options[it->second];
    }

    ParseContext Command::create_context() const noexcept { return ParseContext{}; }

    CliResult<void> Command::apply_defaults(ParseContext &ctx) const
    {
        for (auto &opt : m_options)
        {
            if (!opt.has_default() || ctx.has_value(opt.key_hash()))
                continue;
            auto r = detail::apply_value(
                ctx, opt.key_hash(), opt.value_tag(), opt.default_str());
            if (r.is_err())
                return r;
        }
        return CliResult<void>::Ok();
    }

    CliResult<void> Command::execute(ParseContext &ctx) const
    {
        if (!m_action)
            return CliResult<void>::Ok();
        return m_action(ctx);
    }

    Command &Command::action(std::function<CliResult<void>(ParseContext &)> fn)
    {
        m_action = std::move(fn);
        return *this;
    }

    Command &Command::enabled(std::function<bool()> pred)
    {
        m_enabled = std::move(pred);
        return *this;
    }

    Command &Command::set_visibility(Visibility v)
    {
        m_visibility = v;
        return *this;
    }

    Command &Command::set_extra_args(ExtraArgsPolicy p)
    {
        m_extra_args_policy = p;
        return *this;
    }

}  // namespace pjh::cli
