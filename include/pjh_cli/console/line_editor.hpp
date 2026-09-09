#ifndef INCLUDE_PJH_CLI_CONSOLE_LINE_EDITOR_HPP
#define INCLUDE_PJH_CLI_CONSOLE_LINE_EDITOR_HPP

#include <cstddef>
#include <functional>
#include <istream>
#include <memory>
#include <ostream>
#include <pjh_cli/format/info.hpp>
#include <string>
#include <string_view>
#include <vector>

namespace pjh::cli
{
    /// @brief A single decoded key press.
    struct KeyEvent
    {
        /// @brief Key category; @c Character carries the payload in @c ch.
        enum class Code
        {
            Character,  ///< Printable character in @c ch.
            Enter,      ///< '\r' / '\n'.
            Tab,        ///< '\t'.
            Backspace,  ///< 0x7f / '\b'.
            Up,         ///< Arrow up (reserved for history navigation).
            Down,       ///< Arrow down (reserved for history navigation).
            Eof,        ///< Ctrl-D / stream end (ends the line).
            Unknown,    ///< Escape sequence not understood.
        };

        Code code = Code::Unknown;  ///< Decoded key category.
        char ch = 0;                ///< Printable character for @c Character.
    };

    /// @brief Minimal terminal abstraction consumed by LineEditor.
    ///
    /// Implementations only need single-key reads plus plain-text output; no
    /// ANSI/VT support, cursor addressing, or width detection is required.
    class ITerminal
    {
    public:
        virtual ~ITerminal() = default;

        /// @brief Block until one key is pressed.
        /// @return The decoded key event (never a reference).
        virtual KeyEvent read_key() = 0;

        /// @brief Echo @p text (newlines allowed).
        /// @param text  Text to emit.
        virtual void write(std::string_view text) = 0;

        /// @brief Remove @p count characters already written on the current line.
        /// @param count  Number of characters to erase.
        virtual void erase_last(std::size_t count) = 0;
    };

    /// @brief (line, cursor) -> completion candidates.
    using CompletionFn = std::function<std::vector<CompletionCandidate>(
        std::string_view line, std::size_t cursor)>;

    /// @brief (line, cursor) -> hint text (empty for none).
    using HintFn = std::function<std::string(std::string_view line, std::size_t cursor)>;

    class IHistory;  ///< Forward declaration; full type in console/history.hpp.

    /// @brief Platform-independent interactive line editor.
    ///
    /// Owns the input buffer and dispatches Tab to the completion/hint
    /// callbacks.  Only append-at-end editing is supported (no Left/Right,
    /// no mid-line cursor).  Up/Down navigate the injected IHistory: Up recalls
    /// older entries and Down recalls newer ones, replacing the current buffer.
    /// The line typed before the first Up is kept as a draft and restored when
    /// Down moves past the newest entry.  Without a history, Up/Down are no-ops.
    ///
    /// @note The editor owns neither the terminal nor the history; both must
    ///       outlive it.
    class LineEditor
    {
    public:
        /// @param terminal  Terminal used for key input and echo.  Must outlive
        ///                  the editor.
        /// @param prompt    Prompt echoed once at the start of each line.
        /// @param history   Optional history store; nullptr disables Up/Down.
        ///                  Not owned; must outlive the editor.
        LineEditor(ITerminal &terminal, std::string prompt, IHistory *history = nullptr);

        /// @brief Read one edited line.
        /// @param out       Receives the line (without the trailing newline).
        /// @param complete  Tab completion callback.
        /// @param hint      Hint renderer, called on ambiguous/empty completion.
        /// @return false on EOF/Ctrl-D; true when a line was entered.
        /// @throws Any exception thrown by @p complete or @p hint propagates.
        bool read_line(
            std::string &out, const CompletionFn &complete, const HintFn &hint);

    private:
        /// @brief Handle Tab: complete a unique candidate or print candidates
        ///        and a hint, then redraw.
        void handle_tab(
            std::string &buffer, const CompletionFn &complete, const HintFn &hint);

        /// @brief Print the hint (when non-empty) and redraw prompt + buffer.
        void show_hint(const HintFn &hint, std::string_view buffer);

        /// @brief Print the candidate list, one space-separated line.
        void show_candidates(
            const std::vector<CompletionCandidate> &candidates, std::string_view buffer);

        /// @brief Recall the previous (older) history entry into @p buffer.
        ///        Saves @p buffer as the draft on the first call after a reset.
        /// @param buffer  Current edit buffer, replaced in place.
        void recall_prev(std::string &buffer);

        /// @brief Recall the next (newer) history entry into @p buffer, or the
        ///        saved draft when already past the newest entry.
        /// @param buffer  Current edit buffer, replaced in place.
        void recall_next(std::string &buffer);

        /// @brief Replace @p buffer with @p text, erasing the old rendering.
        /// @param buffer  Current edit buffer, replaced in place.
        /// @param text    New content; must not alias @p buffer.
        void replace_buffer(std::string &buffer, std::string_view text);

        ITerminal &m_terminal;  ///< Non-owning terminal reference.
        std::string m_prompt;   ///< Prompt echoed at the start of each line.

        IHistory *m_history = nullptr;  ///< Non-owning; nullptr disables Up/Down.
        std::string m_draft;            ///< Line typed before the first Up.
        bool m_navigating = false;      ///< True between first Up and reset.
    };

    /// @brief Build a raw-mode TTY terminal for @p input / @p output.
    ///
    /// Returns nullptr unless @p input is exactly std::cin and its descriptor
    /// is an interactive terminal, so injected test streams and non-console
    /// embedders transparently fall back to line-based reading.
    ///
    /// @param input   Input stream to probe (must be std::cin).
    /// @param output  Stream used for echo.
    /// @return Owned terminal, or nullptr when no interactive TTY is available.
    std::unique_ptr<ITerminal> make_tty_terminal(
        std::istream &input, std::ostream &output);
}  // namespace pjh::cli

#endif  // INCLUDE_PJH_CLI_CONSOLE_LINE_EDITOR_HPP
