#ifndef INCLUDE_PJH_CLI_COMMAND_BASE_COMMAND_HPP
#define INCLUDE_PJH_CLI_COMMAND_BASE_COMMAND_HPP

#include <cstddef>
#include <deque>
#include <functional>
#include <memory>
#include <pjh_cli/core/type.hpp>
#include <pjh_cli/detail/concept.hpp>
#include <pjh_cli/detail/help_formatter.hpp>
#include <pjh_cli/detail/meta_flags.hpp>
#include <pjh_cli/detail/string_utils.hpp>
#include <pjh_cli/option/group.hpp>
#include <pjh_cli/option/option_builder.hpp>
#include <pjh_cli/option/option_def.hpp>
#include <pjh_cli/option/option_group_builder.hpp>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

namespace pjh::cli
{

    class ParseContext;
    class BranchCommand;
    class LeafCommand;

    /// @brief Bitmask flags controlling where a Command appears.
    ///
    /// Combine with `|`:
    /// @code
    ///   cmd.set_visibility(Visibility::Cli | Visibility::Repl);
    /// @endcode
    enum class Visibility : unsigned
    {
        Hidden = 0,  ///< Hidden from help / completion everywhere
        Repl = 1,    ///< Visible in interactive REPL only
        Cli = 2,     ///< Visible in batch CLI only
        Both = 3,    ///< Visible everywhere (default)
    };

    /// @brief Bitwise OR for Visibility flags.
    /// @param a First visibility value.
    /// @param b Second visibility value.
    /// @return Combined visibility.
    constexpr Visibility operator|(Visibility a, Visibility b) noexcept
    {
        return static_cast<Visibility>(
            static_cast<unsigned>(a) | static_cast<unsigned>(b));
    }

    /// @brief Bitwise AND for Visibility flags.
    /// @param a First visibility value.
    /// @param b Second visibility value.
    /// @return Intersection of the two flags.
    constexpr Visibility operator&(Visibility a, Visibility b) noexcept
    {
        return static_cast<Visibility>(
            static_cast<unsigned>(a) & static_cast<unsigned>(b));
    }

    /// @brief Policy for handling extra positional arguments beyond registered
    ///        arg<N>.
    enum class ExtraArgsPolicy : unsigned
    {
        Ignore,  ///< Silently discard (default, POSIX convention).
        Error,   ///< Return CliError on any extra positional argument.
        Store,   ///< Append to ParseContext::extra_args() for runtime inspection.
    };

    /// @brief Base node in the command tree.
    ///
    /// Every command can hold:
    ///   - Named options (--flag -o) registered via option<Key>() / option<T, Key>().
    ///
    /// BranchCommand and LeafCommand extend this base.
    ///   - Branch commands have child subcommands (add_branch / add_leaf).
    ///   - Leaf commands have positional arguments (arg<T, Index>()).
    ///
    /// Options are stored polymorphically (unique_ptr<OptionDef>) — the builder
    /// pattern (OptionBuilder) ensures each option is instantiated as the correct
    /// typed subclass (IntOption, BoolOption, etc.).
    class BaseCommand
    {
    public:
        /// @brief Construct a named command.
        /// @param name        Command name (e.g. "serve").
        /// @param description Help text description.
        explicit BaseCommand(std::string name = "", std::string description = "");
        virtual ~BaseCommand() = default;

        BaseCommand(const BaseCommand &) = delete;
        BaseCommand &operator=(const BaseCommand &) = delete;
        BaseCommand(BaseCommand &&) = delete;
        BaseCommand &operator=(BaseCommand &&) = delete;

        // ── Queries ──

        /// @brief Command display name.
        /// @return The value passed at construction, e.g. "serve".
        const std::string &name() const noexcept { return m_name; }

        /// @brief Help description text.
        const std::string &description() const noexcept { return m_description; }

        /// @brief Current visibility flags (default Both).
        Visibility visibility() const noexcept { return m_visibility; }

        /// @brief Evaluate the enabled predicate.
        /// @return true if the command is enabled (i.e., visible to matching).
        bool is_enabled() const { return m_enabled(); }

        /// @brief Parent command (nullptr for the root / App instance).
        BaseCommand *parent() const noexcept { return m_parent; }

        /// @brief Look up an environment value for env-var fallback.
        ///
        /// The parser's finalizer asks the parse root for the value of an
        /// option's env_var().  The default returns nullptr (no environment
        /// available), so a bare BranchCommand root disables env fallback.
        /// The App override reads an environment snapshot captured at
        /// construction; its lookup is heterogeneous and allocates nothing.
        ///
        /// @param name Environment variable name (OptionDef::env_var()).
        /// @return Pointer to the value, or nullptr if absent/unsupported.
        virtual const std::string *env_value(std::string_view name) const
        {
            return nullptr;
        }

        /// @brief Application version string (empty for non-root commands).
        ///        Overridden by App to return its version.
        virtual const std::string &version() const noexcept
        {
            static const std::string empty;
            return empty;
        }

        /// @brief Batch --help / -h renderer for this command (empty = built-in).
        ///
        /// The REPL forwards this to Parser::parse_command() for `cmd --help`, so
        /// an App-injected formatter is honoured interactively as well as in batch.
        /// The default returns an empty function, which selects
        /// detail::default_format_help (HelpFormatter::format_help).  Overridden by
        /// App to return its configured formatter.
        /// @return Current formatter; empty means the built-in renderer.
        virtual const detail::HelpFormatterFn &help_formatter() const noexcept
        {
            static const detail::HelpFormatterFn empty;
            return empty;
        }

        /// @brief Current extra args policy (default Ignore).
        ExtraArgsPolicy extra_args_policy() const noexcept { return m_extra_args_policy; }

        /// @brief Whether set_extra_args() was called explicitly.
        ///
        /// Lets the parser distinguish the implicit `Ignore` default from an
        /// explicit opt-out: on a branch that has subcommands, only the
        /// implicit default is overridden to an unknown-command error.
        /// @return true if the policy was set explicitly.
        bool extra_args_explicit() const noexcept { return m_extra_args_explicit; }

        /// @brief All registered options (pointer-based, polymorphic).
        ///
        /// Discovery surfaces (help, completion, hints) walk the ancestor chain
        /// via detail::collect_options_in_chain; find_option_by_long/short stay
        /// node-local parse queries.
        /// @return Deque of unique_ptr<OptionDef> in registration order.
        const std::deque<std::unique_ptr<OptionDef>> &options() const noexcept
        {
            return m_options;
        }

        /// @brief Look up an option by its long name.
        ///
        /// Current-node query only; parse-time resolution across the ancestor
        /// chain lives in OptionConsumer.
        /// @param name Long option name without "--" prefix (e.g. "verbose").
        /// @return Pointer to OptionDef, or nullptr if not found.
        const OptionDef *find_option_by_long(std::string_view name) const noexcept;

        /// @brief Look up an option by its short character.
        ///
        /// Current-node query only; parse-time resolution across the ancestor
        /// chain lives in OptionConsumer.
        /// @param c Single-character short option, e.g. 'v'.
        /// @return Pointer to OptionDef, or nullptr if not found.
        const OptionDef *find_option_by_short(char c) const noexcept;

        // ── Type queries ──

        /// @brief Downcast to BranchCommand (nullptr if this is a leaf).
        virtual BranchCommand *as_branch() noexcept { return nullptr; }
        /// @brief Const overload.
        virtual const BranchCommand *as_branch() const noexcept { return nullptr; }

        /// @brief Downcast to LeafCommand (nullptr if this is a branch).
        virtual LeafCommand *as_leaf() noexcept { return nullptr; }
        /// @brief Const overload.
        virtual const LeafCommand *as_leaf() const noexcept { return nullptr; }

        /// @brief True if this command can have subcommands.
        bool is_branch() const noexcept { return as_branch() != nullptr; }

        /// @brief True if this command has positional arguments.
        bool is_leaf() const noexcept { return as_leaf() != nullptr; }

        // ── Option registration ──

        /// @brief Register an option without a short name.
        /// @tparam Key Compile-time identifier (fixed_string literal).
        /// @param long_name   Long option name (with or without "--" prefix).
        /// @param description Help text description.
        /// @return OptionBuilder for chaining a type dispatch (.integer() / .boolean() /
        /// …).
        template <auto Key>
            requires detail::OptionKey<decltype(Key)>
        OptionBuilder<Key> option(std::string long_name, std::string description)
        {
            return OptionBuilder<Key>(
                *this, std::move(long_name), std::move(description));
        }

        /// @brief Register an option with a short name.
        /// @tparam Key Compile-time identifier.
        /// @param long_name   Long option name.
        /// @param short_name  Single-character short form (e.g. 'v').
        /// @param description Help text.
        template <auto Key>
            requires detail::OptionKey<decltype(Key)>
        OptionBuilder<Key> option(
            std::string long_name, char short_name, std::string description)
        {
            auto builder =
                OptionBuilder<Key>(*this, std::move(long_name), std::move(description));
            builder.set_short_name(short_name);
            return builder;
        }

        /// @brief Register an option with a default value (type inferred from
        ///        the argument).
        ///
        /// @tparam Key  Compile-time identifier.
        /// @tparam T    Auto-deduced from default_value.
        /// @param long_name   Long option name.
        /// @param description Help text.
        /// @param default_value  Default applied when the option is absent.
        /// @return Reference to the created typed option (as OptionDef&).
        template <auto Key, typename T>
            requires detail::BuiltinType<T>
        OptionDef &option(std::string long_name, std::string description, T default_value)
        {
            auto builder =
                OptionBuilder<Key>(*this, std::move(long_name), std::move(description));
            return detail::dispatch_default<T>(builder, std::move(default_value));
        }

        /// @brief Register an option with short name + default value.
        /// @copydetails option(Key, auto, string, string, T)
        template <auto Key, typename T>
            requires detail::BuiltinType<T>
        OptionDef &option(
            std::string long_name,
            char short_name,
            std::string description,
            T default_value)
        {
            auto builder =
                OptionBuilder<Key>(*this, std::move(long_name), std::move(description));
            builder.set_short_name(short_name);
            return detail::dispatch_default<T>(builder, std::move(default_value));
        }

        // ── Setters ──

        /// @brief Set visibility level (affects help / completion / REPL matching).
        /// @param v Visibility bitmask.
        /// @return *this for chaining.
        BaseCommand &set_visibility(Visibility v);

        /// @brief Set the runtime enable predicate.
        ///
        /// A disabled command is treated as non-existent during matching.
        /// Matching a disabled command produces `command_disabled` error.
        /// @param pred Nullary functor; return false to disable.
        BaseCommand &enabled(std::function<bool()> pred);

        /// @brief Register the action callback invoked when this command is
        ///        matched and parsed successfully.
        ///
        /// The ParseContext is produced and populated by the parse layer
        /// (Parser / ParseFinalizer); this class only passes it through, so
        /// an opaque forward-declared reference suffices.
        /// @param fn Callback receiving the populated ParseContext.
        /// @return *this for chaining.
        BaseCommand &action(std::function<CliResult<void>(ParseContext &)> fn);

        /// @brief Set extra positional args handling policy.
        ///
        /// The policy is inherited by subcommands added afterwards.  Calling
        /// this method (even with Ignore) overrides the parser's default: on a
        /// branch that has subcommands, an unmatched word token is reported as
        /// an unknown command unless a policy was set explicitly.
        /// @param p One of Ignore / Error / Store.
        /// @return *this for chaining.
        BaseCommand &set_extra_args(ExtraArgsPolicy p);

        /// @brief Register an alias name for this command.
        /// @param name Alternative name that also matches this command.
        /// @return *this for chaining.
        BaseCommand &alias(std::string name);

        /// @brief Registered alias names.
        const std::vector<std::string> &aliases() const noexcept { return m_aliases; }

        /// @brief Create a mutually-exclusive / requirement option group.
        ///
        /// Options are referenced by their compile-time key.  The group is
        /// validated after parsing: defaults and env-var values count as "set".
        ///
        /// Usage:
        /// @code
        ///   app.group<fixed_string("port"), fixed_string("socket")>().exactly_one();
        ///   app.group<fixed_string("verbose"), fixed_string("quiet")>().at_most_one();
        ///   app.group<fixed_string("f1"), fixed_string("f2")>().at_least_one();
        /// @endcode
        template <auto... Keys>
            requires(... && detail::OptionKey<decltype(Keys)>)
        auto group()
        {
            return OptionGroupBuilder<Keys...>(*this);
        }

        /// @brief Registered option groups.
        const std::vector<OptionGroup> &groups() const noexcept { return m_groups; }

        /// @brief Register a constructed option group (used by OptionGroupBuilder).
        void register_group(OptionGroup g) { m_groups.push_back(std::move(g)); }

        // ── Lifecycle ──

        /// @brief Execute the registered action callback.
        /// @param ctx Fully populated ParseContext produced by the parse
        ///        layer (Parser / ParseFinalizer).
        /// @return The result of the action callback, or Ok if none
        ///         registered.
        CliResult<void> execute(ParseContext &ctx) const;

    public:
        /// @brief Register an option definition.
        ///
        /// Populates both lookup maps and the ordered option list.
        /// @throws LogicError if the option long name is "help" or "version",
        ///         or the short name is 'h' — those tokens are reserved for the
        ///         built-in --help/-h/--version meta-flags and cannot be
        ///         registered on any command.
        /// @throws LogicError if an option with the same normalized long name or
        ///         the same non-zero short character is already registered on
        ///         this command. The check is per command: an ancestor or
        ///         descendant may reuse a name, and the nearest declaration wins
        ///         at parse time.
        /// @note Strong exception guarantee: on any throw the command is left
        ///       unchanged (no ownership transfer, no dangling lookup entry);
        ///       if indexing throws, the option is removed.
        void add_option(std::unique_ptr<OptionDef> opt)
        {
            if (detail::is_reserved_long_name(opt->long_name()))
                throw LogicError(
                    std::string("BaseCommand::add_option: option '") +
                    opt->display_name() +
                    "' is reserved for the built-in --help/-h/--version flags on command "
                    "'" +
                    m_name + "'");
            if (opt->short_name() != 0 &&
                detail::is_reserved_short_name(opt->short_name()))
                throw LogicError(
                    std::string("BaseCommand::add_option: option '-") +
                    std::string(1, opt->short_name()) +
                    "' is reserved for the built-in --help/-h/--version flags on command "
                    "'" +
                    m_name + "'");

            if (m_option_by_long.contains(opt->long_name()))
                throw LogicError(
                    std::string("BaseCommand::add_option: duplicate long option '") +
                    opt->display_name() + "' on command '" + m_name + "'");
            if (opt->short_name() != 0 && m_option_by_short.contains(opt->short_name()))
                throw LogicError(
                    std::string("BaseCommand::add_option: duplicate short option '-") +
                    std::string(1, opt->short_name()) + "' on command '" + m_name + "'");

            OptionDef *raw = opt.get();
            m_options.push_back(std::move(opt));  // 1) own first

            auto long_it = m_option_by_long.find(raw->long_name());
            OptionDef *prev_long =
                (long_it == m_option_by_long.end()) ? nullptr : long_it->second;
            OptionDef *prev_short = nullptr;
            if (raw->short_name() != 0)
            {
                auto short_it = m_option_by_short.find(raw->short_name());
                if (short_it != m_option_by_short.end())
                    prev_short = short_it->second;
            }

            try
            {
                m_option_by_long[raw->long_name()] = raw;  // 2) index the owned object
                if (raw->short_name() != 0)
                    m_option_by_short[raw->short_name()] = raw;
            }
            catch (...)
            {
                // 3) roll back: restore/erase the entries for raw, then release it.
                if (prev_long != nullptr)
                    m_option_by_long[raw->long_name()] = prev_long;
                else
                    m_option_by_long.erase(raw->long_name());
                if (raw->short_name() != 0)
                {
                    if (prev_short != nullptr)
                        m_option_by_short[raw->short_name()] = prev_short;
                    else
                        m_option_by_short.erase(raw->short_name());
                }
                m_options.pop_back();
                throw;
            }
        }

        /// @brief Look up an option by its key hash.
        const OptionDef *find_option_by_hash(size_t hash) const noexcept
        {
            for (const auto &opt : m_options)
                if (opt->key_hash() == hash)
                    return opt.get();
            return nullptr;
        }

    private:
        friend class BranchCommand;

        /// @brief Set the parent pointer.
        void set_parent(BaseCommand *parent) noexcept { m_parent = parent; }
        std::string m_name;
        std::string m_description;
        BaseCommand *m_parent = nullptr;

        std::deque<std::unique_ptr<OptionDef>> m_options;
        ExtraArgsPolicy m_extra_args_policy = ExtraArgsPolicy::Ignore;
        bool m_extra_args_explicit = false;

        std::unordered_map<
            std::string,
            OptionDef *,
            detail::transparent_string_hash,
            std::equal_to<void>>
            m_option_by_long;
        std::unordered_map<char, OptionDef *> m_option_by_short;

        Visibility m_visibility = Visibility::Both;
        std::function<bool()> m_enabled = []
        {
            return true;
        };
        std::function<CliResult<void>(ParseContext &)> m_action;
        std::vector<std::string> m_aliases;
        std::vector<OptionGroup> m_groups;
    };

}  // namespace pjh::cli

#endif
