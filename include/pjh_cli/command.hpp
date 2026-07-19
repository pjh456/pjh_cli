#ifndef INCLUDE_PJH_CLI_COMMAND_HPP
#define INCLUDE_PJH_CLI_COMMAND_HPP

#include <deque>
#include <filesystem>
#include <functional>
#include <list>
#include <memory>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>

#include "arg_def.hpp"
#include "detail/concept.hpp"
#include "detail/string_utils.hpp"
#include "option_def.hpp"
#include "parse_context.hpp"
#include "type.hpp"

namespace pjh::cli
{

    /// @brief Bitmask flags controlling where a Command appears.
    ///
    /// Combine with `|`: `set_visibility(Visibility::Cli | Visibility::Repl)`
    enum class Visibility : unsigned
    {
        Hidden = 0, ///< Hidden from help / completion everywhere
        Repl = 1,   ///< Visible in interactive REPL
        Cli = 2,    ///< Visible in batch CLI
        Both = 3,   ///< Visible everywhere (default)
    };

    /// @brief Bitwise OR for Visibility flags.
    constexpr Visibility operator|(Visibility a, Visibility b) noexcept
    {
        return static_cast<Visibility>(
            static_cast<unsigned>(a) | static_cast<unsigned>(b));
    }

    /// @brief Bitwise AND for Visibility flags.
    constexpr Visibility operator&(Visibility a, Visibility b) noexcept
    {
        return static_cast<Visibility>(
            static_cast<unsigned>(a) & static_cast<unsigned>(b));
    }

    /// @brief Policy for handling extra positional arguments beyond registered
    ///        arg<N>.
    enum class ExtraArgsPolicy : unsigned
    {
        Ignore, ///< Silently discard (default, POSIX convention).
        Error,  ///< Return CliError on any extra positional argument.
        Store,  ///< Append to ParseContext::extra_args() for runtime inspection.
    };

    // ──────────────────────────────────────────
    //  Forward declarations / helpers
    // ──────────────────────────────────────────

    namespace detail
    {
        /// @brief Dispatch an OptionBuilder to the correct typed subclass and
        ///        set a default value.
        ///
        /// Used by the option<Key>(name, desc, T) auto-dispatch overloads.
        /// @tparam T Computed from the default value argument.
        /// @tparam Builder OptionBuilder<Key> deduced at call site.
        template <typename T, typename Builder>
        OptionDef &dispatch_default(Builder &builder, T default_value)
        {
            if constexpr (std::same_as<T, bool>)
            {
                auto &opt = builder.boolean();
                opt.set_default_str(detail::default_to_string(default_value));
                return opt;
            }
            else if constexpr (std::same_as<T, int>)
            {
                auto &opt = builder.integer();
                opt.set_default_str(detail::default_to_string(default_value));
                return opt;
            }
            else if constexpr (std::same_as<T, double>)
            {
                auto &opt = builder.floating();
                opt.set_default_str(detail::default_to_string(default_value));
                return opt;
            }
            else if constexpr (std::same_as<T, std::string>)
            {
                auto &opt = builder.str();
                opt.set_default_str(detail::default_to_string(default_value));
                return opt;
            }
            else if constexpr (std::same_as<T, std::filesystem::path>)
            {
                auto &opt = builder.path();
                opt.set_default_str(detail::default_to_string(default_value));
                return opt;
            }
        }
    }  // namespace detail

    // ──────────────────────────────────────────
    //  Command class
    // ──────────────────────────────────────────

    /// @brief Composite node in the command tree.
    ///
    /// Every Command can hold:
    ///   - Named options (--flag -o) via option<Key>() / option<T, Key>()
    ///   - Positional arguments via arg<T, Index>()
    ///   - Child subcommands via add_command()
    ///   - An action callback to execute when matched
    ///
    /// App inherits from Command and represents the root node.
    class Command
    {
    public:
        /// @brief Construct a command with optional name and description.
        ///        The root command (App) typically has name set to the program
        ///        name.
        explicit Command(std::string name = "", std::string description = "");

        virtual ~Command() = default;

        Command(const Command &) = delete;
        Command &operator=(const Command &) = delete;
        Command(Command &&) = delete;
        Command &operator=(Command &&) = delete;

        // ──────────────────────────────────────────
        //  Registration — option (new style, Key-only NTTP)
        // ──────────────────────────────────────────

        /// @brief Register an option. Chain .integer()/.boolean()/… to set type.
        /// @tparam Key Compile-time identifier (fixed_string).
        /// @return OptionBuilder for attaching a type dispatch and further
        ///         chain calls.
        template <auto Key>
            requires detail::OptionKey<decltype(Key)>
        OptionBuilder<Key> option(std::string long_name, std::string description)
        {
            return OptionBuilder<Key>(
                *this, std::move(long_name), std::move(description));
        }

        /// @brief Register an option with a short name.
        /// @tparam Key Compile-time identifier (fixed_string).
        /// @param short_name Single-character short form (e.g. 'v').
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

        /// @brief Register an option with a default value.
        ///
        /// Dispatches to the correct typed subclass based on the type of
        /// @p default_value (int → IntOption, bool → BoolOption, etc.).
        /// @tparam Key  Compile-time identifier.
        /// @tparam T    Auto-deduced from default_value.
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

        // ──────────────────────────────────────────
        //  Registration — other
        // ──────────────────────────────────────────

        /// @brief Register a positional argument identified by compile-time Index.
        /// @tparam T Value type.
        /// @tparam Index Positional index (0, 1, 2, …).
        template <typename T, size_t Index>
            requires detail::BuiltinType<T>
        ArgDef &arg(std::string name, std::string description);

        /// @brief Add a child subcommand. Returns reference to the child.
        Command &add_command(std::string name, std::string description);

        /// @brief Set the action callback invoked when this command is matched.
        Command &action(std::function<CliResult<void>(ParseContext &)> fn);

        /// @brief Set the runtime enable predicate.
        ///        A disabled command is treated as non-existent during matching.
        Command &enabled(std::function<bool()> pred);

        /// @brief Set visibility level (affects help / REPL matching).
        Command &set_visibility(Visibility v);

        /// @brief Set extra positional args handling policy (default: Ignore).
        Command &set_extra_args(ExtraArgsPolicy p);

        // ──────────────────────────────────────────
        //  Queries
        // ──────────────────────────────────────────

        /// @brief Command name (e.g. "serve").
        const std::string &name() const noexcept { return m_name; }

        /// @brief Help description.
        const std::string &description() const noexcept { return m_description; }

        /// @brief Current visibility flags.
        Visibility visibility() const noexcept { return m_visibility; }

        /// @brief Evaluate the enabled predicate.
        bool is_enabled() const { return m_enabled(); }

        /// @brief Parent command (nullptr for root).
        Command *parent() const noexcept { return m_parent; }

        /// @brief Direct child subcommands.
        const std::list<Command> &subcommands() const noexcept { return m_subcommands; }

        /// @brief All registered options (pointer-based, polymorphic).
        const std::deque<std::unique_ptr<OptionDef>> &options() const noexcept
        {
            return m_options;
        }

        /// @brief Registered positional arguments.
        const std::deque<ArgDef> &args() const noexcept { return m_args; }

        /// @brief Current extra args policy.
        ExtraArgsPolicy extra_args_policy() const noexcept { return m_extra_args_policy; }

        /// @brief Find a direct child subcommand by exact name match.
        Command *find_subcommand(std::string_view name) noexcept;

        /// @brief Const overload.
        const Command *find_subcommand(std::string_view name) const noexcept;

        /// @brief Look up an option by its long name (without -- prefix).
        const OptionDef *find_option_by_long(std::string_view name) const noexcept;

        /// @brief Look up an option by its short character.
        const OptionDef *find_option_by_short(char c) const noexcept;

        // ──────────────────────────────────────────
        //  Parser / lifecycle helpers
        // ──────────────────────────────────────────

        /// @brief Create an empty ParseContext for this command.
        ParseContext create_context() const noexcept;

        /// @brief Pre-fill context with default values from registered options.
        CliResult<void> apply_defaults(ParseContext &ctx) const;

        /// @brief Execute the registered action callback.
        CliResult<void> execute(ParseContext &ctx) const;

        /// @brief Internal — store an option object.
        ///
        /// Registers the option in the by-long-name and by-short-name lookup
        /// maps, then transfers ownership into m_options.  Called by
        /// OptionBuilder and the auto-dispatch option<Key>() overloads.
        void add_option(std::unique_ptr<OptionDef> opt)
        {
            m_option_by_long[opt->long_name()] = opt.get();
            if (opt->short_name() != 0)
                m_option_by_short[opt->short_name()] = opt.get();
            m_options.push_back(std::move(opt));
        }

    private:
        std::string m_name;
        std::string m_description;
        Command *m_parent = nullptr;

        std::deque<std::unique_ptr<OptionDef>> m_options;
        std::deque<ArgDef> m_args;
        std::list<Command> m_subcommands;

        std::unordered_map<
            std::string,
            OptionDef *,
            detail::transparent_string_hash,
            std::equal_to<void>>
            m_option_by_long;
        std::unordered_map<char, OptionDef *> m_option_by_short;

        ExtraArgsPolicy m_extra_args_policy = ExtraArgsPolicy::Ignore;
        Visibility m_visibility = Visibility::Both;
        std::function<bool()> m_enabled = []
        {
            return true;
        };
        std::function<CliResult<void>(ParseContext &)> m_action;
    };

    // ──────────────────────────────────────────────
    //  arg<T, Index>
    // ──────────────────────────────────────────────

    template <typename T, size_t Index>
        requires detail::BuiltinType<T>
    ArgDef &Command::arg(std::string name, std::string description)
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

    // ──────────────────────────────────────────────
    //  OptionBuilder method definitions
    //  (defined here so Command::add_option is visible)
    // ──────────────────────────────────────────────

    /// @brief Create the option as an integer-valued type and store it.
    template <auto Key>
        requires detail::OptionKey<decltype(Key)>
    IntOption &OptionBuilder<Key>::integer()
    {
        auto name = m_long_name;
        if (name.size() > 2 && name[0] == '-' && name[1] == '-')
            name = name.substr(2);

        auto ptr = std::make_unique<IntOption>();
        ptr->set_long_name(std::move(name));
        ptr->set_short_name(m_short_name);
        ptr->set_description(std::move(m_description));
        ptr->set_has_value(true);
        ptr->set_value_tag(ValueTag::Int);
        ptr->set_key_hash(key_hash(Key));

        auto &ref = *ptr;
        m_cmd.add_option(std::move(ptr));
        return ref;
    }

    /// @brief Create the option as a boolean flag and store it.
    template <auto Key>
        requires detail::OptionKey<decltype(Key)>
    BoolOption &OptionBuilder<Key>::boolean()
    {
        auto name = m_long_name;
        if (name.size() > 2 && name[0] == '-' && name[1] == '-')
            name = name.substr(2);

        auto ptr = std::make_unique<BoolOption>();
        ptr->set_long_name(std::move(name));
        ptr->set_short_name(m_short_name);
        ptr->set_description(std::move(m_description));
        ptr->set_has_value(false);
        ptr->set_value_tag(ValueTag::Bool);
        ptr->set_key_hash(key_hash(Key));

        auto &ref = *ptr;
        m_cmd.add_option(std::move(ptr));
        return ref;
    }

    /// @brief Create the option as a string-valued type and store it.
    template <auto Key>
        requires detail::OptionKey<decltype(Key)>
    StrOption &OptionBuilder<Key>::str()
    {
        auto name = m_long_name;
        if (name.size() > 2 && name[0] == '-' && name[1] == '-')
            name = name.substr(2);

        auto ptr = std::make_unique<StrOption>();
        ptr->set_long_name(std::move(name));
        ptr->set_short_name(m_short_name);
        ptr->set_description(std::move(m_description));
        ptr->set_has_value(true);
        ptr->set_value_tag(ValueTag::String);
        ptr->set_key_hash(key_hash(Key));

        auto &ref = *ptr;
        m_cmd.add_option(std::move(ptr));
        return ref;
    }

    /// @brief Create the option as a double-valued type and store it.
    template <auto Key>
        requires detail::OptionKey<decltype(Key)>
    FloatOption &OptionBuilder<Key>::floating()
    {
        auto name = m_long_name;
        if (name.size() > 2 && name[0] == '-' && name[1] == '-')
            name = name.substr(2);

        auto ptr = std::make_unique<FloatOption>();
        ptr->set_long_name(std::move(name));
        ptr->set_short_name(m_short_name);
        ptr->set_description(std::move(m_description));
        ptr->set_has_value(true);
        ptr->set_value_tag(ValueTag::Double);
        ptr->set_key_hash(key_hash(Key));

        auto &ref = *ptr;
        m_cmd.add_option(std::move(ptr));
        return ref;
    }

    /// @brief Create the option as a filesystem path type and store it.
    template <auto Key>
        requires detail::OptionKey<decltype(Key)>
    PathOption &OptionBuilder<Key>::path()
    {
        auto name = m_long_name;
        if (name.size() > 2 && name[0] == '-' && name[1] == '-')
            name = name.substr(2);

        auto ptr = std::make_unique<PathOption>();
        ptr->set_long_name(std::move(name));
        ptr->set_short_name(m_short_name);
        ptr->set_description(std::move(m_description));
        ptr->set_has_value(true);
        ptr->set_value_tag(ValueTag::Path);
        ptr->set_key_hash(key_hash(Key));

        auto &ref = *ptr;
        m_cmd.add_option(std::move(ptr));
        return ref;
    }

}  // namespace pjh::cli

#endif  // INCLUDE_PJH_CLI_COMMAND_HPP
