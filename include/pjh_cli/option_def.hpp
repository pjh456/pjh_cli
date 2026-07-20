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
    class BaseCommand;

    // ── Forward declarations for typed subclasses ──

    /// @cond FORWARD_DECLS
    class IntOption;
    class CountOption;
    class BoolOption;
    class StrOption;
    class FloatOption;
    class PathOption;
    /// @endcond

    // ──────────────────────────────────────────
    //  OptionDef — base class
    // ──────────────────────────────────────────

    /// @brief Base class for all option definitions.
    ///
    /// Holds common fields (name, description, key hash, required flag, etc.)
    /// and provides virtual `parse_value()` and `apply_default()` that each
    /// typed subclass overrides.  Options are stored polymorphically in
    /// \`BaseCommand::m_options\` as \`unique_ptr<OptionDef>\`.
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

        /// @brief Long option name (e.g. "verbose").
        const std::string &long_name() const noexcept { return m_long_name; }

        /// @brief Short option character (0 if none).
        char short_name() const noexcept { return m_short_name; }

        /// @brief Help text description.
        const std::string &description() const noexcept { return m_description; }

        /// @brief Whether this option consumes a value token.
        virtual bool has_value() const noexcept { return m_has_value; }

        /// @brief Whether this option must appear on the command line.
        bool is_required() const noexcept { return m_required; }

        /// @brief Compile-time hash used to index ParseContext.
        size_t key_hash() const noexcept { return m_key_hash; }

        /// @brief Runtime type tag for value conversion.
        ValueTag value_tag() const noexcept { return m_value_tag; }

        /// @brief Registered completer callback (empty if none).
        const std::function<std::vector<std::string>()> &completer_fn() const noexcept
        {
            return m_completer;
        }

        /// @brief Whether a typed default value has been registered.
        virtual bool has_default() const noexcept { return false; }

        /// @brief Whether this option counts occurrences (-vvv → 3).
        virtual bool is_counting() const noexcept { return false; }

        /// @brief Whether this option supports --no-xxx negation.
        virtual bool is_negatable() const noexcept { return false; }

        /// @brief Whether this option accepts repeated values.
        bool is_repeatable() const noexcept { return m_repeatable; }

        /// @brief Parse a raw CLI token and store the typed value in @p ctx.
        /// @param ctx Parse context to write into.
        /// @param raw The raw string value from the command line.
        /// @return Ok on success, or a CliError (type conversion failure).
        virtual CliResult<void> parse_value(
            ParseContext &ctx, std::string_view raw) const;

        /// @brief Apply the default value into @p ctx if no value is present.
        /// @param ctx Parse context to write into.
        /// @return Ok on success, or a CliError (default value parse failure).
        virtual CliResult<void> apply_default(ParseContext &ctx) const;

        // ── Chainable setters ──

        /// @brief Set the long option name.
        OptionDef &set_long_name(const std::string &s)
        {
            m_long_name = s;
            return *this;
        }

        /// @brief Set the long option name (move).
        OptionDef &set_long_name(std::string &&s)
        {
            m_long_name = std::move(s);
            return *this;
        }

        /// @brief Set the short option character (0 for none).
        OptionDef &set_short_name(char c)
        {
            m_short_name = c;
            return *this;
        }

        /// @brief Set the help text description.
        OptionDef &set_description(const std::string &s)
        {
            m_description = s;
            return *this;
        }

        /// @brief Set the help text description (move).
        OptionDef &set_description(std::string &&s)
        {
            m_description = std::move(s);
            return *this;
        }

        /// @brief Set whether this option consumes a value token.
        OptionDef &set_has_value(bool v)
        {
            m_has_value = v;
            return *this;
        }

        /// @brief Set the environment variable name for fallback.
        OptionDef &env(std::string var)
        {
            m_env_var = std::move(var);
            return *this;
        }

        /// @brief Environment variable name (empty if none).
        const std::string &env_var() const noexcept { return m_env_var; }

        /// @brief Set the compile-time hash for ParseContext indexing.
        OptionDef &set_key_hash(size_t h)
        {
            m_key_hash = h;
            return *this;
        }

        /// @brief Set the runtime type tag.
        OptionDef &set_value_tag(ValueTag t)
        {
            m_value_tag = t;
            return *this;
        }

        /// @brief Mark this option as required.
        OptionDef &required(bool r = true)
        {
            m_required = r;
            return *this;
        }

        /// @brief Enable repeated values (--opt a --opt b).
        /// @note Must be called before parse_value / apply_default.
        OptionDef &repeatable()
        {
            m_repeatable = true;
            return *this;
        }

        /// @brief Register a completer function for shell completion.
        OptionDef &completer(std::function<std::vector<std::string>()> fn)
        {
            m_completer = std::move(fn);
            return *this;
        }

    protected:
        /// @brief Store @p value (or append if repeatable).
        template <typename T>
        CliResult<void> store_or_append(ParseContext &ctx, size_t hash, T value) const
        {
            if (m_repeatable)
                ctx.append_value(hash, std::move(value));
            else
                ctx.set_value(hash, std::move(value));
            return CliResult<void>::Ok();
        }

        std::string m_long_name;
        std::string m_env_var;
        char m_short_name{};
        std::string m_description;
        bool m_has_value{};
        bool m_required{};
        bool m_repeatable{};
        size_t m_key_hash{};
        ValueTag m_value_tag{};
        std::function<std::vector<std::string>()> m_completer;
    };

    // ── OptionBuilder (declaration; definitions in command.hpp) ──

    /// @brief Builder returned by BaseCommand::option<Key>().
    ///
    /// Holds common registration fields and provides type-dispatch methods
    /// (.integer(), .boolean(), .str(), …) that create the corresponding
    /// typed subclass and add it to the command.  The methods are defined in
    /// `base_command.hpp` so that BaseCommand::add_option() is visible.
    /// @tparam Key Compile-time fixed_string identifier.
    template <auto Key>
        requires detail::OptionKey<decltype(Key)>
    class OptionBuilder
    {
        BaseCommand &m_cmd;
        std::string m_long_name;
        std::string m_description;
        char m_short_name = 0;

    public:
        /// @brief Construct a builder for the given command and option name.
        /// @param cmd        Target command to register the option on.
        /// @param long_name  Long option name (with or without "--" prefix).
        /// @param description Help text description.
        OptionBuilder(BaseCommand &cmd, std::string long_name, std::string description) :
            m_cmd(cmd),
            m_long_name(std::move(long_name)),
            m_description(std::move(description))
        {
        }

        /// @brief Set the short option character.
        /// @param c Single-character short form (e.g. 'v'), or 0 for none.
        void set_short_name(char c) noexcept { m_short_name = c; }

        /// @brief Create the option as an integer-valued type.
        /// @return Reference to the newly created IntOption.
        IntOption &integer();

        /// @brief Create the option as a counting flag (-vvv → 3).
        /// @return Reference to the newly created CountOption.
        CountOption &count();

        /// @brief Create the option as a boolean flag type.
        /// @return Reference to the newly created BoolOption.
        BoolOption &boolean();

        /// @brief Create the option as a string-valued type.
        /// @return Reference to the newly created StrOption.
        StrOption &str();

        /// @brief Create the option as a double-valued floating-point type.
        /// @return Reference to the newly created FloatOption.
        FloatOption &floating();

        /// @brief Create the option as a filesystem path type.
        /// @return Reference to the newly created PathOption.
        PathOption &path();

    private:
        template <typename Opt, ValueTag Tag>
        Opt &make_option(bool has_val);
    };

    // ── Virtual method default implementations ──

    /// @brief Default: option does not accept a value; returns an error.
    inline CliResult<void> OptionDef::parse_value(ParseContext &, std::string_view) const
    {
        return CliFailure{CliError("option does not accept a value")};
    }

    /// @brief Default: no-op (no default value to apply).
    inline CliResult<void> OptionDef::apply_default(ParseContext &) const
    {
        return CliResult<void>::Ok();
    }

}  // namespace pjh::cli

#endif  // INCLUDE_PJH_CLI_OPTION_DEF_HPP
