#ifndef INCLUDE_PJH_CLI_PARSE_DETAIL_PARSE_CONTEXT_WRITER_HPP
#define INCLUDE_PJH_CLI_PARSE_DETAIL_PARSE_CONTEXT_WRITER_HPP

#include <memory>
#include <pjh_cli/core/type.hpp>
#include <pjh_cli/parse/parse_context.hpp>
#include <string>

namespace pjh::cli::detail
{
    /// @brief Internal writer API for populating a ParseContext during
    ///        command-line parsing.
    ///
    /// @internal Parse-pipeline implementation detail; not part of the public API.
    ///
    /// Parser components (OptionConsumer, SubcommandResolver, ValueWriter,
    /// ParseFinalizer, Parser) use this class to write into a ParseContext
    /// without exposing write methods on ParseContext's public interface.
    ///
    /// All methods are static; the class cannot be instantiated.
    ///
    /// ParseContextWriter is a friend of ParseContext and accesses its
    /// private storage directly.
    ///
    /// Usage:
    /// @code
    ///   ParseContextWriter::set_value<int>(ctx, hash, 42);
    ///   ParseContextWriter::set_parent(ctx, parent_shared_ptr);
    /// @endcode
    class ParseContextWriter
    {
    public:
        ParseContextWriter() = delete;

        /// @brief Store a single typed value by runtime key hash.
        template <detail::BuiltinType T>
        static void set_value(ParseContext &ctx, size_t hash, T value)
        {
            ctx.scalar_map<T>()[hash] = std::move(value);
        }

        /// @brief Append a value for a repeatable option.
        template <detail::BuiltinType T>
        static void append_value(ParseContext &ctx, size_t hash, T value)
        {
            ctx.vector_map<T>()[hash].push_back(std::move(value));
        }

        /// @brief Check if a key hash is present (walks parent chain).
        ///
        /// Presence is derived from the value storage (scalar and vector
        /// maps); no separate presence index is maintained.
        static bool has_value(const ParseContext &ctx, size_t hash) noexcept
        {
            return ctx.has_in_chain(hash);
        }

        /// @brief Read a typed value by runtime hash with a fallback.
        template <detail::BuiltinType T>
        static T get_value(const ParseContext &ctx, size_t hash, T default_val)
        {
            if (auto *p = ctx.find_scalar<T>(hash))
                return *p;
            return default_val;
        }

        /// @brief Borrow the parent context one level up the descent chain.
        ///
        /// Returns nullptr at the root.  Used by OptionConsumer to route a
        /// value to the context of the command that declares the option, so
        /// repeatable/counting ancestor options accumulate regardless of where
        /// the token appears on the command line.
        /// @param ctx Context whose parent is requested.
        /// @return Non-owning pointer to the parent, or nullptr at the root.
        static ParseContext *parent_of(ParseContext &ctx) noexcept
        {
            return ctx.m_parent.get();
        }

        /// @brief Link a parent context for scoped subcommand lookup.
        static void set_parent(
            ParseContext &ctx, std::shared_ptr<ParseContext> parent) noexcept
        {
            ctx.m_parent = std::move(parent);
        }

        /// @brief Record the deepest matched command.
        static void set_matched_command(ParseContext &ctx, BaseCommand *cmd) noexcept
        {
            ctx.m_matched_cmd = cmd;
        }

        /// @brief Store pre-formatted help text from --help handling.
        static void set_help_text(ParseContext &ctx, std::string text)
        {
            ctx.m_help_text = std::move(text);
        }

        /// @brief Store pre-formatted version text from --version handling.
        static void set_version_text(ParseContext &ctx, std::string text)
        {
            ctx.m_version_text = std::move(text);
        }

        /// @brief Append an unrecognised positional argument.
        ///
        /// The token is routed to the root of @p ctx's parent chain so extra
        /// args accumulate across subcommand descent instead of being stranded
        /// in an ancestor context that the caller never observes.
        static void add_extra_arg(ParseContext &ctx, std::string s)
        {
            ctx.root_context().m_extra_args.push_back(std::move(s));
        }
    };

}  // namespace pjh::cli::detail

#endif
