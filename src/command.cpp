#include <algorithm>
#include <pjh_cli/command.hpp>
#include <sstream>

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
        return it->second;
    }

    const OptionDef *Command::find_option_by_short(char c) const noexcept
    {
        auto it = m_option_by_short.find(c);
        if (it == m_option_by_short.end())
            return nullptr;
        return it->second;
    }

    ParseContext Command::create_context() const noexcept { return ParseContext{}; }

    CliResult<void> Command::apply_defaults(ParseContext &ctx) const
    {
        for (const auto &opt_ptr : m_options)
        {
            if (!opt_ptr->has_default() || ctx.has_value(opt_ptr->key_hash()))
                continue;
            auto r = opt_ptr->apply_default(ctx);
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

    std::vector<Command *> ParseContext::matched_commands()
    {
        if (!m_matched_cmd) return {};
        std::vector<Command *> result;
        for (auto *c = m_matched_cmd; c; c = c->parent())
            result.push_back(c);
        std::reverse(result.begin(), result.end());
        if (result.size() > 1)
            result.erase(result.begin());
        return result;
    }

    std::vector<const Command *> ParseContext::matched_commands() const
    {
        if (!m_matched_cmd) return {};
        std::vector<const Command *> result;
        for (auto *c = m_matched_cmd; c; c = c->parent())
            result.push_back(c);
        std::reverse(result.begin(), result.end());
        if (result.size() > 1)
            result.erase(result.begin());
        return result;
    }

    std::string ParseContext::matched_path() const
    {
        auto cmds = matched_commands();
        if (cmds.empty()) return {};
        std::ostringstream os;
        os << cmds[0]->name();
        for (size_t i = 1; i < cmds.size(); ++i)
            os << ' ' << cmds[i]->name();
        return os.str();
    }

}  // namespace pjh::cli
