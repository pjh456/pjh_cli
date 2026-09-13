#ifndef INCLUDE_PJH_CLI_ERROR_HPP
#define INCLUDE_PJH_CLI_ERROR_HPP

#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

// The formatter (format_error) and the ErrorFactory methods are defined
// out-of-line in src/error.cpp, so this header no longer pulls in <format>.
namespace pjh::cli
{

    // ── Error detail structs ─────────────────────────────────────────

    /// @brief Raw message with no structure. Escape hatch for simple errors.
    struct RawMessageError
    {
        std::string message;
    };

    /// @brief Generic parse error at a specific argument position.
    struct ParseError
    {
        std::string raw_input;
        int position;
    };

    /// @brief User specified an option that was not registered.
    ///
    /// `suggestions` is empty unless the producer found close long-option
    /// names worth suggesting.
    struct UnknownOptionError
    {
        std::string option_display;            // "--foo" or "-f"
        std::vector<std::string> suggestions;  ///< Fuzzy candidates, closest first.
    };

    /// @brief Option declared as taking a value, but none provided.
    struct MissingValueError
    {
        std::string option_display;  // "--port" or "-p"
    };

    /// @brief Required option was not present on the command line.
    struct MissingRequiredOptionError
    {
        std::string option_name;  // "--port"
    };

    /// @brief Required positional argument was not provided.
    struct MissingRequiredArgError
    {
        std::string arg_name;  // "file"
    };

    /// @brief String value could not be converted to the expected type.
    struct TypeConversionError
    {
        std::string option_display;  // "--port" or positional arg name ("file")
        std::string raw_value;
        std::string expected_type;  // "integer", "float", "bool (…)"
    };

    /// @brief Multiple commands matched the input (fuzzy match ambiguity).
    struct AmbiguousCommandError
    {
        std::string input;
        std::vector<std::string> candidates;
    };

    /// @brief A word token did not match any subcommand of a dispatcher branch.
    struct UnknownCommandError
    {
        std::string input;
        std::vector<std::string> suggestions;  ///< Fuzzy candidates, closest first.
    };

    /// @brief Value is outside the allowed range [min, max].
    struct ValueOutOfRangeError
    {
        std::string option_display;  // "--port"
        std::string raw_value;
        std::string min;
        std::string max;
    };

    /// @brief String value did not match any valid enum mapping.
    struct EnumValueError
    {
        std::string option_display;  // "--color"
        std::string raw_value;
        std::vector<std::string> valid_choices;
    };

    /// @brief Command exists but is currently disabled.
    struct CommandDisabledError
    {
        std::string command_name;
    };

    /// @brief Multiple options from a mutually-exclusive group were provided.
    struct ConflictingOptionsError
    {
        std::vector<std::string> option_names;
    };

    /// @brief A required option group was not satisfied.
    struct RequiredOptionGroupError
    {
        std::vector<std::string> option_names;
        bool exactly_one;  // true=ExactlyOne, false=AtLeastOne
    };

    /// @brief Attempted to pass a value to a flag-style option.
    struct OptionDoesNotAcceptValueError
    {
        std::string option_display;
    };

    /// @brief The parser finished but no command was matched.
    struct NoCommandMatchedError
    {
    };

    // ── ErrorInfo variant ────────────────────────────────────────────

    /// @brief Type-safe variant covering all possible parse errors.
    using ErrorInfo = std::variant<
        RawMessageError,
        ParseError,
        UnknownOptionError,
        MissingValueError,
        MissingRequiredOptionError,
        MissingRequiredArgError,
        TypeConversionError,
        AmbiguousCommandError,
        UnknownCommandError,
        ValueOutOfRangeError,
        EnumValueError,
        CommandDisabledError,
        ConflictingOptionsError,
        RequiredOptionGroupError,
        OptionDoesNotAcceptValueError,
        NoCommandMatchedError>;

    // ── format_error — centralised ───────────────────────────────────

    namespace detail
    {
        /// @brief Dependent-false helper for compile-time exhaustive
        ///        `if constexpr` dispatch chains.
        ///
        /// Always `false`, but the value depends on the template parameter, so
        /// a `static_assert(always_false_v<T>)` inside a discarded `else`
        /// branch only fires once that branch is instantiated for an unhandled
        /// type.  Used by format_error() and detail::dispatch_default().
        /// @tparam Ts Ignored; present so the name is usable as a pack.
        template <typename... Ts>
        inline constexpr bool always_false_v = false;
    }

    /// @brief Render an ErrorInfo variant to its human-readable message.
    ///
    /// Exhaustive by construction: the visitor has a final dependent
    /// `static_assert`, so adding an ErrorInfo alternative without a matching
    /// branch is a compile-time error here, not undefined behaviour.
    /// @param info Structured error payload.
    /// @return Rendered message without the "Parse Error: " prefix.
    std::string format_error(const ErrorInfo &info);

    // ── CliError ─────────────────────────────────────────────────────

    /// @brief Category of a CliError; drives the `what()` prefix.
    enum class ErrorKind
    {
        Parse,   ///< Command-line parsing / validation failure.
        Runtime  ///< Action/execution failure reported by the embedder.
    };

    /// @brief Stable error tag for localisation and diagnostic dispatch.
    ///
    /// One value per ErrorInfo alternative, in declaration order, plus Runtime for
    /// errors whose ErrorKind is Runtime.  This is an append-only contract: never
    /// renumber or reuse an existing value, so downstream mappings (e.g. message
    /// catalogues) stay valid across releases.  Adding an ErrorInfo alternative
    /// requires appending a matching tag; CliError::tag() enforces it with a
    /// dependent static_assert.  Use CliError::tag() / ErrorDiagnostic::kind()
    /// rather than the coarser CliError::kind().
    enum class ErrorTag : unsigned
    {
        RawMessage = 0,                 ///< RawMessageError: unstructured message.
        Parse = 1,                      ///< ParseError: generic parse failure.
        UnknownOption = 2,              ///< UnknownOptionError.
        MissingValue = 3,               ///< MissingValueError.
        MissingRequiredOption = 4,      ///< MissingRequiredOptionError.
        MissingRequiredArg = 5,         ///< MissingRequiredArgError.
        TypeConversion = 6,             ///< TypeConversionError.
        AmbiguousCommand = 7,           ///< AmbiguousCommandError.
        UnknownCommand = 8,             ///< UnknownCommandError.
        ValueOutOfRange = 9,            ///< ValueOutOfRangeError.
        EnumValue = 10,                 ///< EnumValueError.
        CommandDisabled = 11,           ///< CommandDisabledError.
        ConflictingOptions = 12,        ///< ConflictingOptionsError.
        RequiredOptionGroup = 13,       ///< RequiredOptionGroupError.
        OptionDoesNotAcceptValue = 14,  ///< OptionDoesNotAcceptValueError.
        NoCommandMatched = 15,          ///< NoCommandMatchedError.
        Runtime = 16,                   ///< ErrorKind::Runtime, any ErrorInfo.
    };

    class ErrorDiagnostic;

    /// @brief Error type for parse and runtime failures.
    ///
    /// Stores a structured ErrorInfo variant plus an ErrorKind category.
    /// Parse errors (constructed from an ErrorInfo, e.g. via ErrorFactory)
    /// render `what()` as "Parse Error: " + format_error(info).  Runtime
    /// errors (constructed from a plain string, or via
    /// ErrorFactory::runtime_error) render the message only, with no
    /// prefix.  Use kind() to introspect the category instead of parsing
    /// `what()`.
    ///
    /// Note: wrapping a runtime message in ErrorInfo(RawMessageError{...})
    /// and passing it to the one-argument constructor yields a Parse error.
    class CliError : public std::runtime_error
    {
        ErrorInfo m_info;
        ErrorKind m_kind;

        static std::string render_what(const ErrorInfo &info, ErrorKind kind);

    public:
        /// @brief Construct a parse error from a structured ErrorInfo variant.
        explicit CliError(ErrorInfo info) : CliError(std::move(info), ErrorKind::Parse) {}

        /// @brief Construct an error from a structured ErrorInfo and explicit kind.
        /// @param info Structured error payload.
        /// @param kind Category controlling the `what()` prefix.  Deliberately has
        ///        no default, so the one-argument ErrorInfo overload is unambiguous.
        CliError(ErrorInfo info, ErrorKind kind) :
            std::runtime_error(render_what(info, kind)),
            m_info(std::move(info)),
            m_kind(kind)
        {
        }

        /// @brief Construct a runtime error from a plain string.
        ///
        /// The message is stored as RawMessageError but categorised as
        /// ErrorKind::Runtime, so `what()` returns the message with no
        /// "Parse Error: " prefix.
        explicit CliError(const std::string &msg) :
            CliError(ErrorInfo(RawMessageError{msg}), ErrorKind::Runtime)
        {
        }

        /// @brief Construct a runtime error from a C-string.
        explicit CliError(const char *msg) : CliError(std::string(msg)) {}

        /// @brief Access the structured error information.
        const ErrorInfo &info() const noexcept { return m_info; }

        /// @brief Access the coarse-grained error category (drives the `what()` prefix).
        ///
        /// This is the ErrorKind (`Parse` / `Runtime`) consumed by `App::run()`'s
        /// exit-code mapping and the `"Parse Error: "` prefix; it is deliberately
        /// coarse.  For the stable, localisation-friendly tag, use tag() or
        /// ErrorDiagnostic::kind().
        ErrorKind kind() const noexcept { return m_kind; }

        /// @brief Diagnostic message: byte-for-byte equal to `what()`.
        ///
        /// Exposes the pjh_result `Diagnostic` protocol so CliError works with
        /// pjh::result::render() and pjh::result::Context<CliError>.  Parse errors
        /// therefore keep the `"Parse Error: "` prefix; use format_error(info())
        /// for the prefix-free body.  The returned view aliases this error object
        /// and is valid for its lifetime.
        std::string_view message() const noexcept { return what(); }

        /// @brief Stable, fine-grained tag for localisation.
        ///
        /// Returns ErrorTag::Runtime when kind() is ErrorKind::Runtime, otherwise
        /// the tag matching the active ErrorInfo alternative.  Unlike kind(), this
        /// value is append-only and safe to switch over in user renderers.
        ErrorTag tag() const noexcept;

        /// @brief Adapt this error to a pjh_result Diagnostic with a fine-grained kind().
        ///
        /// @return Non-owning view; valid only while this error object is alive.
        ErrorDiagnostic diagnostic() const noexcept;
    };

    /// @brief Non-owning view adapting a CliError to the pjh_result Diagnostic protocol.
    ///
    /// `message()` mirrors CliError::what(), while `kind()` exposes the stable
    /// ErrorTag (not ErrorKind), so generic rendering and localisation can switch
    /// on tags without visiting ErrorInfo variants.  error_kind() and info()
    /// forward the coarse category and the structured payload.
    ///
    /// @note This is a non-owning view: it stores only a pointer to the source
    ///       CliError.  Neither the view nor any string_view returned by
    ///       message() may outlive that error object.
    class ErrorDiagnostic
    {
        const CliError *m_error;

    public:
        /// @brief Bind to a CliError, which must outlive this view.
        explicit ErrorDiagnostic(const CliError &e) noexcept : m_error(&e) {}

        /// @brief Diagnostic message, byte-for-byte equal to the source `what()`.
        ///
        /// Parse errors include the `"Parse Error: "` prefix.
        std::string_view message() const noexcept { return m_error->what(); }

        /// @brief Stable ErrorTag for localisation (see CliError::tag()).
        ErrorTag kind() const noexcept { return m_error->tag(); }

        /// @brief The coarse-grained ErrorKind of the source error.
        ErrorKind error_kind() const noexcept { return m_error->kind(); }

        /// @brief The structured error payload of the source error.
        const ErrorInfo &info() const noexcept { return m_error->info(); }
    };

    // ── LogicError ───────────────────────────────────────────────────

    /// @brief Error type for programming mistakes (not parse failures).
    ///
    /// Thrown when the library API is used incorrectly, e.g. calling
    /// ParseContext::get() without checking has() first.
    class LogicError : public std::logic_error
    {
    public:
        using std::logic_error::logic_error;
    };

    // ── ErrorFactory ─────────────────────────────────────────────────

    /// @brief Utility class for creating CliError instances.
    ///
    /// All methods return a fully-formed CliError storing a structured
    /// ErrorInfo variant.  The human-readable `what()` string matches
    /// the variant content.
    ///
    /// Usage:
    /// @code
    ///   return CliFailure{ErrorFactory::unknown_option("--foo")};
    /// @endcode
    class ErrorFactory
    {
    public:
        ErrorFactory() = delete;

        static CliError parse_error(std::string_view arg_name, int position);

        static CliError unknown_option(std::string_view display);

        /// @brief Build an unknown-option error with fuzzy suggestions.
        /// @param display     The option as typed (e.g. "--prot").
        /// @param suggestions  Close candidate displays, closest first
        ///        (e.g. {"--port"}); when empty, renders the plain message.
        /// @return CliError rendering "unknown option: '<display>'; did you
        ///         mean: …" when suggestions are present.
        static CliError unknown_option(
            std::string_view display, const std::vector<std::string> &suggestions);

        static CliError missing_value(std::string_view display);

        static CliError missing_required_option(std::string_view name);

        static CliError missing_required_arg(std::string_view name);

        static CliError type_conversion_error(
            std::string_view name,
            std::string_view value,
            std::string_view expected_type);

        static CliError ambiguous_command(
            std::string_view input, const std::vector<std::string> &candidates);

        static CliError unknown_command(
            std::string_view input, const std::vector<std::string> &suggestions);

        static CliError value_out_of_range(
            std::string_view name, std::string_view value, int min, int max);

        static CliError value_out_of_range(
            std::string_view name, std::string_view value, double min, double max);

        static CliError enum_value_error(
            std::string_view display,
            std::string_view raw,
            const std::vector<std::string> &valid);

        static CliError command_disabled(std::string_view name);

        static CliError conflicting_options(const std::vector<std::string> &names);

        static CliError required_option_group(
            const std::vector<std::string> &names, bool exactly_one);

        static CliError option_does_not_accept_value(std::string_view display);

        static CliError no_command_matched();

        /// @brief Build a runtime/execution error.  `what()` is the message only.
        static CliError runtime_error(std::string_view message);
    };

}  // namespace pjh::cli

#endif  // INCLUDE_PJH_CLI_ERROR_HPP
