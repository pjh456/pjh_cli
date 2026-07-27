#ifndef INCLUDE_PJH_CLI_PARSE_PARSE_CONTEXT_WRITER_HPP
#define INCLUDE_PJH_CLI_PARSE_PARSE_CONTEXT_WRITER_HPP

#include <memory>
#include <pjh_cli/core/type.hpp>
#include <pjh_cli/parse/parse_context.hpp>
#include <string>

namespace pjh::cli
{
    class BaseCommand;

    /// @brief Internal writer API for populating a ParseContext during
    ///        command-line parsing.
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
            ctx.m_present.insert(hash);
        }

        /// @brief Append a value for a repeatable option.
        template <detail::BuiltinType T>
        static void append_value(ParseContext &ctx, size_t hash, T value)
        {
            ctx.vector_map<T>()[hash].push_back(std::move(value));
            ctx.m_present.insert(hash);
        }

        /// @brief Check if a key hash is present (walks parent chain).
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
        static void add_extra_arg(ParseContext &ctx, std::string s)
        {
            ctx.m_extra_args.push_back(std::move(s));
        }
    };

}  // namespace pjh::cli

#endif
