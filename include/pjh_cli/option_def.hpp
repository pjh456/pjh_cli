#ifndef INCLUDE_PJH_CLI_OPTION_DEF_HPP
#define INCLUDE_PJH_CLI_OPTION_DEF_HPP

#include <functional>
#include <pjh_result.hpp>
#include <string>
#include <string_view>
#include <vector>

#include "fixed_string.hpp"
#include "parse_context.hpp"
#include "type.hpp"

namespace pjh::cli
{
    class ParseContext;

    // ── Forward declarations for typed subclasses ──

    class IntOption;
    class BoolOption;
    class StrOption;
    class FloatOption;
    class PathOption;

    // ──────────────────────────────────────────
    //  OptionDef — base class
    // ──────────────────────────────────────────

    /// @brief Base class for all option definitions.
    ///
    /// Holds common fields and provides virtual `parse_value()` and
    /// `apply_default()` that each typed subclass overrides.
    class OptionDef
    {
    public:
        OptionDef() = default;
        virtual ~OptionDef() = default;

        OptionDef(const OptionDef &) = delete;
        OptionDef &operator=(const OptionDef &) = delete;
        OptionDef(OptionDef &&) = delete;
        OptionDef &operator=(OptionDef &&) = delete;

        // ── Getters ──

        const std::string &long_name() const noexcept { return m_long_name; }
        char short_name() const noexcept { return m_short_name; }
        const std::string &description() const noexcept { return m_description; }
        bool has_value() const noexcept { return m_has_value; }
        bool is_required() const noexcept { return m_required; }
        size_t key_hash() const noexcept { return m_key_hash; }
        ValueTag value_tag() const noexcept { return m_value_tag; }
        const std::function<std::vector<std::string>()> &completer_fn() const noexcept
        {
            return m_completer;
        }

        /// @brief Whether a typed default value has been registered.
        virtual bool has_default() const noexcept { return false; }

        /// @brief Parse a raw token and store the typed value in @p ctx.
        virtual CliResult<void> parse_value(ParseContext &, std::string_view) const;

        /// @brief Apply the default value into @p ctx (if no value present).
        virtual CliResult<void> apply_default(ParseContext &) const;

        // ── Chainable setters ──

        OptionDef &set_long_name(const std::string &s)
        {
            m_long_name = s;
            return *this;
        }
        OptionDef &set_long_name(std::string &&s)
        {
            m_long_name = std::move(s);
            return *this;
        }
        OptionDef &set_short_name(char c)
        {
            m_short_name = c;
            return *this;
        }
        OptionDef &set_description(const std::string &s)
        {
            m_description = s;
            return *this;
        }
        OptionDef &set_description(std::string &&s)
        {
            m_description = std::move(s);
            return *this;
        }
        OptionDef &set_has_value(bool v)
        {
            m_has_value = v;
            return *this;
        }
        OptionDef &set_key_hash(size_t h)
        {
            m_key_hash = h;
            return *this;
        }
        OptionDef &set_value_tag(ValueTag t)
        {
            m_value_tag = t;
            return *this;
        }
        OptionDef &required(bool r = true)
        {
            m_required = r;
            return *this;
        }
        OptionDef &completer(std::function<std::vector<std::string>()> fn)
        {
            m_completer = std::move(fn);
            return *this;
        }

    protected:
        std::string m_long_name;
        char m_short_name{};
        std::string m_description;
        bool m_has_value{};
        bool m_required{};
        size_t m_key_hash{};
        ValueTag m_value_tag{};
        std::function<std::vector<std::string>()> m_completer;
    };

    // ── OptionBuilder (declaration; definitions in command.hpp) ──

    /// @brief Builder returned by Command::option<Key>().
    template <auto Key>
        requires detail::OptionKey<decltype(Key)>
    class OptionBuilder
    {
        Command &m_cmd;
        std::string m_long_name;
        std::string m_description;
        char m_short_name = 0;

    public:
        OptionBuilder(Command &cmd, std::string long_name, std::string description) :
            m_cmd(cmd),
            m_long_name(std::move(long_name)),
            m_description(std::move(description))
        {
        }

        void set_short_name(char c) noexcept { m_short_name = c; }

        IntOption &integer();
        BoolOption &boolean();
        StrOption &str();
        FloatOption &floating();
        PathOption &path();
    };

    // ── Virtual method default implementations ──

    inline CliResult<void> OptionDef::parse_value(ParseContext &, std::string_view) const
    {
        return CliFailure{CliError("option does not accept a value")};
    }

    inline CliResult<void> OptionDef::apply_default(ParseContext &) const
    {
        return CliResult<void>::Ok();
    }

}  // namespace pjh::cli

#endif  // INCLUDE_PJH_CLI_OPTION_DEF_HPP
