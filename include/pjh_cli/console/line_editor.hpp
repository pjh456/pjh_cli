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
            Left,       ///< Arrow left: move the cursor one code point left.
            Right,      ///< Arrow right: move the cursor one code point right.
            Home,       ///< Home: move the cursor to the start of the line.
            End,        ///< End: move the cursor to the end of the line.
            Delete,     ///< Delete: remove the code point under the cursor.
            WordLeft,   ///< Ctrl/Alt-Left: move the cursor one word left.
            WordRight,  ///< Ctrl/Alt-Right: move the cursor one word right.
            Cancel,     ///< Ctrl-C: discard the current line and start over.
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

        /// @brief Remove @p count display characters already written on the
        ///        current line.
        ///
        /// @param count  Number of UTF-8 code points to erase (not bytes).
        ///               Each is rendered with one `"\b \b"` sequence, so a
        ///               double-width glyph occupies two columns and is only
        ///               partially cleared by one unit (see @note).
        ///
        /// @note Counts code points, not terminal columns; no Unicode width
        ///       table is consulted.  Callers must pass
        ///       `detail::utf8_code_point_count(text)` for a full-buffer
        ///       redraw.  ASCII input is unaffected.
        virtual void erase_last(std::size_t count) = 0;

        /// @brief Move the cursor @p count code points left without erasing.
        ///
        /// @param count  Number of UTF-8 code points to move back.
        ///
        /// @note Counts code points, not terminal columns (same caveat as
        ///       erase_last); ASCII input is unaffected.  The default
        ///       implementation writes one ASCII backspace per code point, so
        ///       a backend that only supports plain-text output needs no
        ///       override and no ANSI/VT support is required.
        virtual void move_cursor_left(std::size_t count)
        {
            for (std::size_t i = 0; i < count; ++i) write("\b");
        }

        /// @brief Restore cooked terminal mode (line discipline + echo).
        ///
        /// Called before running a human-in-the-loop action that reads stdin,
        /// so input is visible and backspace works.  No-op for non-TTY
        /// terminals; idempotent when the terminal is not in raw mode.
        virtual void suspend() {}

        /// @brief Re-enter raw single-key mode after suspend().
        ///
        /// No-op for non-TTY terminals; idempotent when already raw.
        virtual void resume() {}
    };

    /// @brief (line, cursor) -> completion candidates and the matched prefix
    ///        length.  The editor appends `candidate.substr(prefix_len)` when a
    ///        unique candidate is inserted, so inline `--opt=value` and compact
    ///        `-pVALUE` forms complete correctly.
    using CompletionFn =
        std::function<CompletionResult(std::string_view line, std::size_t cursor)>;

    /// @brief (line, cursor) -> hint text (empty for none).
    using HintFn = std::function<std::string(std::string_view line, std::size_t cursor)>;

    class IHistory;  ///< Forward declaration; full type in console/history.hpp.

    /// @brief Platform-independent interactive line editor.
    ///
    /// Owns the input buffer and dispatches Tab to the completion/hint
    /// callbacks.  Editing happens on UTF-8 code-point boundaries with a
    /// movable cursor: Left/Right step one code point, Home/End jump to the
    /// start/end, Delete removes the code point under the cursor, and
    /// Ctrl/Alt-Left/Right move one word at a time (a word is an ASCII
    /// alphanumeric run, `_`, or any non-ASCII code point).  A mid-line edit
    /// redraws the buffer from its start and repositions the visible cursor
    /// through @c ITerminal::move_cursor_left.
    ///
    /// Up/Down navigate the injected IHistory: Up recalls older entries and
    /// Down recalls newer ones, replacing the current buffer and moving the
    /// cursor to its end.  The line typed before the first Up is kept as a draft
    /// and restored when Down moves past the newest entry.  Without a history,
    /// Up/Down are no-ops.
    ///
    /// Ctrl-C (@c KeyEvent::Code::Cancel) discards the current line, echoes
    /// @c ^C, resets history navigation, and starts a fresh prompt without
    /// returning; the REPL keeps running.  Ctrl-D (@c KeyEvent::Code::Eof)
    /// ends input and makes read_line() return false.
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

        /// @brief Redraw @p new_buffer from the start of the buffer region and
        ///        park the visible cursor at @p new_cursor.
        ///
        /// Precondition: the hardware cursor is at the current @c m_cursor and
        /// the visible line still shows the old buffer.  Uses only
        /// @c move_cursor_left + @c write, so no cursor-addressing/ANSI support
        /// is needed.
        ///
        /// @param new_buffer   Content to render.
        /// @param new_cursor   Byte offset in @p new_buffer for the cursor; must
        ///                     sit on a UTF-8 code-point boundary.
        void render(std::string_view new_buffer, std::size_t new_cursor);

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

        std::size_t m_cursor = 0;        ///< Cursor byte offset into the buffer.
        std::size_t m_cursor_cp = 0;     ///< Cursor offset in UTF-8 code points.
        std::size_t m_rendered_len = 0;  ///< Rendered buffer length (code points).
    };

    namespace detail
    {
        /// @brief Whether raw-mode interactive editing is available.
        ///
        /// Raw mode is only safe on the process console: the POSIX and Windows
        /// backends read the stdin descriptor directly and echo through the
        /// passed output stream, so a redirected stdout would receive the
        /// prompt/echo while the user's keystrokes are consumed invisibly.
        /// Injected test streams and non-console embedders must fall back to
        /// std::getline.
        ///
        /// @param input_is_stdin    @p input is exactly @c std::cin.
        /// @param output_is_stdout  @p output is exactly @c std::cout.
        /// @param stdin_is_tty      fd 0 is an interactive terminal.
        /// @param stdout_is_tty     fd 1 is an interactive terminal.
        /// @return true only when all four hold.
        constexpr bool raw_mode_available(
            bool input_is_stdin,
            bool output_is_stdout,
            bool stdin_is_tty,
            bool stdout_is_tty) noexcept
        {
            return input_is_stdin && output_is_stdout && stdin_is_tty && stdout_is_tty;
        }
    }  // namespace detail

    /// @brief Build a raw-mode TTY terminal for @p input / @p output.
    ///
    /// Returns nullptr unless @p input / @p output are exactly std::cin /
    /// std::cout and both descriptors are interactive terminals, so injected
    /// test streams and non-console embedders transparently fall back to
    /// line-based reading.  A redirected stdout (e.g. `app > log`) disables raw
    /// mode and falls back to std::getline: the prompt is still written to that
    /// output stream once per line, while keys are read without raw-mode echo.
    ///
    /// @param input   Input stream to probe (must be std::cin).
    /// @param output  Stream used for echo (must be std::cout).
    /// @return Owned terminal, or nullptr when no interactive TTY is available.
    std::unique_ptr<ITerminal> make_tty_terminal(
        std::istream &input, std::ostream &output);
}  // namespace pjh::cli

#endif  // INCLUDE_PJH_CLI_CONSOLE_LINE_EDITOR_HPP
