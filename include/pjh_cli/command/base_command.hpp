#ifndef INCLUDE_PJH_CLI_COMMAND_BASE_COMMAND_HPP
#define INCLUDE_PJH_CLI_COMMAND_BASE_COMMAND_HPP

#include <concepts>
#include <deque>
#include <filesystem>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>

#include "../detail/concept.hpp"
#include "../detail/string_utils.hpp"
#include "../option/bool_option.hpp"
#include "../option/count_option.hpp"
#include "../option/float_option.hpp"
#include "../option/int_option.hpp"
#include "../option/path_option.hpp"
#include "../option/str_option.hpp"
#include "../option_def.hpp"
#include "../parse_context.hpp"
#include "../type.hpp"

namespace pjh::cli
{

    class BranchCommand;
    class LeafCommand;

    enum class Visibility : unsigned
    {
        Hidden = 0,
        Repl = 1,
        Cli = 2,
        Both = 3,
    };

    constexpr Visibility operator|(Visibility a, Visibility b) noexcept
    {
        return static_cast<Visibility>(
            static_cast<unsigned>(a) | static_cast<unsigned>(b));
    }

    constexpr Visibility operator&(Visibility a, Visibility b) noexcept
    {
        return static_cast<Visibility>(
            static_cast<unsigned>(a) & static_cast<unsigned>(b));
    }

    enum class ExtraArgsPolicy : unsigned
    {
        Ignore,
        Error,
        Store,
    };

    namespace detail
    {
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

    class BaseCommand
    {
    public:
        explicit BaseCommand(std::string name = "", std::string description = "");
        virtual ~BaseCommand() = default;

        BaseCommand(const BaseCommand &) = delete;
        BaseCommand &operator=(const BaseCommand &) = delete;
        BaseCommand(BaseCommand &&) = delete;
        BaseCommand &operator=(BaseCommand &&) = delete;

        // ── Queries ──

        const std::string &name() const noexcept { return m_name; }
        const std::string &description() const noexcept { return m_description; }
        Visibility visibility() const noexcept { return m_visibility; }
        bool is_enabled() const { return m_enabled(); }
        BaseCommand *parent() const noexcept { return m_parent; }
        ExtraArgsPolicy extra_args_policy() const noexcept { return m_extra_args_policy; }
        const std::deque<std::unique_ptr<OptionDef>> &options() const noexcept
        {
            return m_options;
        }

        const OptionDef *find_option_by_long(std::string_view name) const noexcept;
        const OptionDef *find_option_by_short(char c) const noexcept;

        // ── Type queries ──

        virtual BranchCommand *as_branch() noexcept { return nullptr; }
        virtual const BranchCommand *as_branch() const noexcept { return nullptr; }
        virtual LeafCommand *as_leaf() noexcept { return nullptr; }
        virtual const LeafCommand *as_leaf() const noexcept { return nullptr; }
        bool is_branch() const noexcept { return as_branch() != nullptr; }
        bool is_leaf() const noexcept { return as_leaf() != nullptr; }

        // ── Option registration ──

        template <auto Key>
            requires detail::OptionKey<decltype(Key)>
        OptionBuilder<Key> option(std::string long_name, std::string description)
        {
            return OptionBuilder<Key>(
                *this, std::move(long_name), std::move(description));
        }

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

        template <auto Key, typename T>
            requires detail::BuiltinType<T>
        OptionDef &option(std::string long_name, std::string description, T default_value)
        {
            auto builder =
                OptionBuilder<Key>(*this, std::move(long_name), std::move(description));
            return detail::dispatch_default<T>(builder, std::move(default_value));
        }

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

        BaseCommand &set_visibility(Visibility v);
        BaseCommand &enabled(std::function<bool()> pred);
        BaseCommand &action(std::function<CliResult<void>(ParseContext &)> fn);
        BaseCommand &set_extra_args(ExtraArgsPolicy p);

        // ── Lifecycle ──

        ParseContext create_context() const noexcept;
        CliResult<void> apply_defaults(ParseContext &ctx) const;
        CliResult<void> execute(ParseContext &ctx) const;

    public:
        /// @brief Internal — register an option definition.
        /// @note Public because OptionBuilder and derived command types need access.
        void add_option(std::unique_ptr<OptionDef> opt)
        {
            m_option_by_long[opt->long_name()] = opt.get();
            if (opt->short_name() != 0)
                m_option_by_short[opt->short_name()] = opt.get();
            m_options.push_back(std::move(opt));
        }

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

}  // namespace pjh::cli

#endif
