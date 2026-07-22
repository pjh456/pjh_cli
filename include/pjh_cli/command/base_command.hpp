#ifndef INCLUDE_PJH_CLI_COMMAND_BASE_COMMAND_HPP
#define INCLUDE_PJH_CLI_COMMAND_BASE_COMMAND_HPP

#include <concepts>
#include <deque>
#include <filesystem>
#include <functional>
#include <memory>
#include <pjh_cli/core/type.hpp>
#include <pjh_cli/detail/concept.hpp>
#include <pjh_cli/detail/string_utils.hpp>
#include <pjh_cli/option/bool_option.hpp>
#include <pjh_cli/option/count_option.hpp>
#include <pjh_cli/option/enum_option.hpp>
#include <pjh_cli/option/float_option.hpp>
#include <pjh_cli/option/int_option.hpp>
#include <pjh_cli/option/option_def.hpp>
#include <pjh_cli/option/path_option.hpp>
#include <pjh_cli/option/str_option.hpp>
#include <pjh_cli/parse/parse_context.hpp>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>

namespace pjh::cli
{

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

    namespace detail
    {
        /// @brief Dispatch an OptionBuilder to the correct typed subclass and
        ///        set a default value.
        ///
        /// Used by the convenience option<Key>(name, desc, T) overloads that
        /// infer the option type from the default value argument type.
        /// @tparam T Computed from the default value argument (bool → BoolOption,
        ///           int → IntOption, string → StrOption, etc.).
        /// @tparam Builder OptionBuilder<Key> deduced at call site.
        template <typename T, typename Builder>
            requires detail::BuiltinType<T>
        OptionDef &dispatch_default(Builder &builder, T default_value)
        {
            if constexpr (std::same_as<T, bool>)
                return builder.boolean().default_value(default_value);
            else if constexpr (std::same_as<T, int>)
                return builder.integer().default_value(default_value);
            else if constexpr (std::same_as<T, double>)
                return builder.floating().default_value(default_value);
            else if constexpr (std::same_as<T, std::string>)
                return builder.str().default_value(default_value);
            else if constexpr (std::same_as<T, std::filesystem::path>)
                return builder.path().default_value(default_value);
        }
    }  // namespace detail

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

        /// @brief Application version string (empty for non-root commands).
        ///        Overridden by App to return its version.
        virtual const std::string &version() const noexcept
        {
            static const std::string empty;
            return empty;
        }

        /// @brief Current extra args policy (default Ignore).
        ExtraArgsPolicy extra_args_policy() const noexcept { return m_extra_args_policy; }

        /// @brief All registered options (pointer-based, polymorphic).
        /// @return Deque of unique_ptr<OptionDef> in registration order.
        const std::deque<std::unique_ptr<OptionDef>> &options() const noexcept
        {
            return m_options;
        }

        /// @brief Look up an option by its long name.
        /// @param name Long option name without "--" prefix (e.g. "verbose").
        /// @return Pointer to OptionDef, or nullptr if not found.
        const OptionDef *find_option_by_long(std::string_view name) const noexcept;

        /// @brief Look up an option by its short character.
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
        /// @param fn Callback receiving the populated ParseContext.
        /// @return *this for chaining.
        BaseCommand &action(std::function<CliResult<void>(ParseContext &)> fn);

        /// @brief Set extra positional args handling policy.
        /// @param p One of Ignore / Error / Store.
        /// @return *this for chaining.
        BaseCommand &set_extra_args(ExtraArgsPolicy p);

        // ── Lifecycle ──

        /// @brief Create an empty ParseContext for this command.
        ParseContext create_context() const noexcept;

        /// @brief Pre-fill context with default values from registered options.
        ///
        /// Iterates all options; for each option with has_default() and no
        /// user-supplied value, calls opt->apply_default().
        /// @param ctx Parse context to modify.
        /// @return Ok or Err if a default value failed to parse.
        CliResult<void> apply_defaults(ParseContext &ctx) const;

        /// @brief Execute the registered action callback.
        /// @param ctx Fully populated ParseContext from the parser.
        /// @return The result of the action callback, or Ok if none registered.
        CliResult<void> execute(ParseContext &ctx) const;

    public:
        /// @brief Register an option definition.
        ///
        /// Populates both lookup maps and the ordered option list.
        void add_option(std::unique_ptr<OptionDef> opt)
        {
            m_option_by_long[opt->long_name()] = opt.get();
            if (opt->short_name() != 0)
                m_option_by_short[opt->short_name()] = opt.get();
            m_options.push_back(std::move(opt));
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
    };

    // ── OptionBuilder method definitions ──

    /// @brief Common factory: strip leading "--", create typed option,
    ///        register on command.
    /// @tparam Opt Concrete option type (e.g. IntOption).
    /// @tparam Tag Matching ValueTag.
    /// @param has_val Whether the option consumes a value token.
    /// @return Reference to the newly registered option.
    template <auto Key>
        requires detail::OptionKey<decltype(Key)>
    template <typename Opt, ValueTag Tag>
    Opt &OptionBuilder<Key>::make_option(bool has_val)
    {
        auto name = m_long_name;
        if (name.size() > 2 && name[0] == '-' && name[1] == '-')
            name = name.substr(2);

        auto ptr = std::make_unique<Opt>();
        ptr->set_long_name(std::move(name));
        ptr->set_short_name(m_short_name);
        ptr->set_description(std::move(m_description));
        ptr->set_has_value(has_val);
        ptr->set_value_tag(Tag);
        ptr->set_key_hash(key_hash(Key));

        auto &ref = *ptr;
        m_cmd.add_option(std::move(ptr));
        return ref;
    }

    template <auto Key>
        requires detail::OptionKey<decltype(Key)>
    IntOption &OptionBuilder<Key>::integer()
    {
        return make_option<IntOption, ValueTag::Int>(true);
    }

    template <auto Key>
        requires detail::OptionKey<decltype(Key)>
    CountOption &OptionBuilder<Key>::count()
    {
        return make_option<CountOption, ValueTag::Int>(false);
    }

    template <auto Key>
        requires detail::OptionKey<decltype(Key)>
    BoolOption &OptionBuilder<Key>::boolean()
    {
        return make_option<BoolOption, ValueTag::Bool>(false);
    }

    template <auto Key>
        requires detail::OptionKey<decltype(Key)>
    StrOption &OptionBuilder<Key>::str()
    {
        return make_option<StrOption, ValueTag::String>(true);
    }

    template <auto Key>
        requires detail::OptionKey<decltype(Key)>
    FloatOption &OptionBuilder<Key>::floating()
    {
        return make_option<FloatOption, ValueTag::Double>(true);
    }

    template <auto Key>
        requires detail::OptionKey<decltype(Key)>
    PathOption &OptionBuilder<Key>::path()
    {
        return make_option<PathOption, ValueTag::Path>(true);
    }

    template <auto Key>
        requires detail::OptionKey<decltype(Key)>
    template <typename E>
        requires std::is_enum_v<E>
    EnumOption<E> &OptionBuilder<Key>::enum_type()
    {
        return make_option<EnumOption<E>, ValueTag::Int>(true);
    }

}  // namespace pjh::cli

#endif
