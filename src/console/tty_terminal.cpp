#include <cstddef>
#include <iostream>
#include <memory>
#include <optional>
#include <ostream>
#include <pjh_cli/console/line_editor.hpp>
#include <pjh_cli/detail/io_retry.hpp>
#include <pjh_platform/console.hpp>
#include <string>
#include <string_view>
#include <utility>

#if defined(_WIN32)
#include <conio.h>
#else
#include <poll.h>
#include <unistd.h>
#endif

namespace
{
#if defined(_WIN32)

    /// @brief Windows console backend built on _getch().
    ///
    /// Saves the console input mode on construction, switches it to raw
    /// (echo, line input, and processed input disabled) and restores it in the
    /// destructor.  Clearing @c ENABLE_PROCESSED_INPUT keeps Ctrl-C in the input
    /// buffer so it arrives as byte 0x03 and is mapped to
    /// @c KeyEvent::Code::Cancel instead of being handled by the system.
    class WindowsTerminal final : public pjh::cli::ITerminal
    {
    public:
        /// @param output  Stream used for echo.
        explicit WindowsTerminal(std::ostream &output) : m_output(output)
        {
            auto r = pjh::platform::ConsoleMode::make_raw(0);
            if (r.is_ok())
                m_mode.emplace(std::move(r).unwrap());
        }

        void suspend() override
        {
            if (m_mode)
                (void)m_mode->suspend();
        }

        void resume() override
        {
            if (m_mode)
                (void)m_mode->resume();
        }

        pjh::cli::KeyEvent read_key() override
        {
            int c = ::_getch();
            if (c == 0x03)  // Ctrl-C: cancel the current line.
                return {pjh::cli::KeyEvent::Code::Cancel, 0};
            if (c == 0x04)  // Ctrl-D: end of input.
                return {pjh::cli::KeyEvent::Code::Eof, 0};
            if (c == 0xE0 || c == 0x00)  // Extended key prefix.
            {
                using Code = pjh::cli::KeyEvent::Code;
                int ext = ::_getch();
                switch (ext)
                {
                case 'H':
                    return {Code::Up, 0};
                case 'P':
                    return {Code::Down, 0};
                case 'K':
                    return {Code::Left, 0};
                case 'M':
                    return {Code::Right, 0};
                case 'G':
                    return {Code::Home, 0};
                case 'O':
                    return {Code::End, 0};
                case 'S':
                    return {Code::Delete, 0};
                case 0x73:  // Ctrl-Left.
                    return {Code::WordLeft, 0};
                case 0x74:  // Ctrl-Right.
                    return {Code::WordRight, 0};
                default:
                    return {Code::Unknown, 0};
                }
            }
            if (c == '\r' || c == '\n')
                return {pjh::cli::KeyEvent::Code::Enter, 0};
            if (c == '\t')
                return {pjh::cli::KeyEvent::Code::Tab, 0};
            if (c == 0x7f || c == '\b')
                return {pjh::cli::KeyEvent::Code::Backspace, 0};
            if (c < 0x20)
                return {pjh::cli::KeyEvent::Code::Unknown, 0};
            return {pjh::cli::KeyEvent::Code::Character, static_cast<char>(c)};
        }

        void write(std::string_view text) override { m_output << text << std::flush; }

        void erase_last(std::size_t count) override
        {
            // One erase sequence per display character (code point); the
            // caller passes a code-point count, not a byte count.
            for (std::size_t i = 0; i < count; ++i) m_output << "\b \b";
            m_output << std::flush;
        }

    private:
        std::ostream &m_output;
        std::optional<pjh::platform::ConsoleMode> m_mode;
    };

#else

    /// @brief POSIX raw-mode backend built on read().
    ///
    /// Saves the terminal attributes on construction and restores them in the
    /// destructor, so every exit path (EOF, quit, exception) leaves the shell
    /// in a usable state.  Raw mode also clears @c ISIG, so the kernel does not
    /// generate SIGINT/SIGQUIT/SIGTSTP; Ctrl-C arrives as byte 0x03 and is
    /// mapped to @c KeyEvent::Code::Cancel (Ctrl-Z / Ctrl-\\ become inert).
    /// suspend() / resume() temporarily hand the terminal back to cooked mode
    /// for human-in-the-loop actions (with @c ISIG restored) and re-enter raw
    /// mode afterwards.  Restoration only happens when the terminal was
    /// actually switched to raw mode.
    class PosixTerminal final : public pjh::cli::ITerminal
    {
    public:
        /// @param output  Stream used for echo.
        explicit PosixTerminal(std::ostream &output) : m_output(output)
        {
            auto r = pjh::platform::ConsoleMode::make_raw(STDIN_FILENO);
            if (r.is_ok())
                m_mode.emplace(std::move(r).unwrap());
        }

        void suspend() override
        {
            if (m_mode)
                (void)m_mode->suspend();
        }

        void resume() override
        {
            if (m_mode)
                (void)m_mode->resume();
        }

        PosixTerminal(const PosixTerminal &) = delete;
        PosixTerminal &operator=(const PosixTerminal &) = delete;

        pjh::cli::KeyEvent read_key() override
        {
            char c = 0;
            // EINTR is retried inside the helper; a true EOF (0) and any other
            // read error (< 0) still end the session.
            const auto n = pjh::cli::detail::retry_on_eintr(
                [&c] { return ::read(STDIN_FILENO, &c, 1); });
            if (n != 1)
                return {pjh::cli::KeyEvent::Code::Eof, 0};

            switch (c)
            {
            case 0x03:
                return {pjh::cli::KeyEvent::Code::Cancel, 0};
            case '\r':
            case '\n':
                return {pjh::cli::KeyEvent::Code::Enter, 0};
            case '\t':
                return {pjh::cli::KeyEvent::Code::Tab, 0};
            case 0x7f:
            case '\b':
                return {pjh::cli::KeyEvent::Code::Backspace, 0};
            case 0x04:
                return {pjh::cli::KeyEvent::Code::Eof, 0};
            case 0x1b:
                return read_escape();
            default:
                if (static_cast<unsigned char>(c) < 0x20)
                    return {pjh::cli::KeyEvent::Code::Unknown, 0};
                return {pjh::cli::KeyEvent::Code::Character, c};
            }
        }

        void write(std::string_view text) override { m_output << text << std::flush; }

        void erase_last(std::size_t count) override
        {
            // One erase sequence per display character (code point); the
            // caller passes a code-point count, not a byte count.
            for (std::size_t i = 0; i < count; ++i) m_output << "\b \b";
            m_output << std::flush;
        }

    private:
        /// @brief Decode an ESC-prefixed sequence without blocking on a bare ESC
        ///        (short poll between bytes).
        ///
        /// Handles the SS3 (@c ESC @c O) and CSI (@c ESC @c [) forms, including
        /// parameterized CSI sequences such as @c "[1;5D" and @c "[3~".  Every
        /// byte of a recognized sequence is consumed so parameter bytes never
        /// leak into the input buffer as characters; unrecognized sequences map
        /// to @c Unknown.  Alt-b / Alt-f map to word movement.
        pjh::cli::KeyEvent read_escape()
        {
            using Code = pjh::cli::KeyEvent::Code;
            char first = 0;
            if (!read_with_timeout(first))
                return {Code::Unknown, 0};  // Bare ESC.
            if (first == 'b')
                return {Code::WordLeft, 0};
            if (first == 'f')
                return {Code::WordRight, 0};
            if (first == 'O')
            {
                char second = 0;
                if (!read_with_timeout(second))
                    return {Code::Unknown, 0};
                switch (second)
                {
                case 'A':
                    return {Code::Up, 0};
                case 'B':
                    return {Code::Down, 0};
                case 'C':
                    return {Code::Right, 0};
                case 'D':
                    return {Code::Left, 0};
                case 'H':
                    return {Code::Home, 0};
                case 'F':
                    return {Code::End, 0};
                default:
                    return {Code::Unknown, 0};
                }
            }
            if (first != '[')
                return {Code::Unknown, 0};

            // CSI: collect parameter bytes 0x20..0x3F until the final byte
            // 0x40..0x7E.  The cap keeps a malformed stream from stalling.
            std::string params;
            char final = 0;
            for (;;)
            {
                char byte = 0;
                if (!read_with_timeout(byte))
                    return {Code::Unknown, 0};
                const auto ub = static_cast<unsigned char>(byte);
                if (ub >= 0x40 && ub <= 0x7E)
                {
                    final = byte;
                    break;
                }
                if (ub < 0x20 || ub > 0x3F || params.size() >= 8)
                    return {Code::Unknown, 0};
                params.push_back(byte);
            }
            return decode_csi(params, final);
        }

        /// @brief Map collected CSI parameters plus the final byte to a KeyEvent.
        /// @param params  Parameter bytes (digits and ';'), empty when none.
        /// @param final   Final byte in 0x40..0x7E.
        /// @return Decoded key, or @c Unknown for an unmapped sequence.
        static pjh::cli::KeyEvent decode_csi(std::string_view params, char final)
        {
            using Code = pjh::cli::KeyEvent::Code;
            if (params.empty())
            {
                switch (final)
                {
                case 'A':
                    return {Code::Up, 0};
                case 'B':
                    return {Code::Down, 0};
                case 'C':
                    return {Code::Right, 0};
                case 'D':
                    return {Code::Left, 0};
                case 'H':
                    return {Code::Home, 0};
                case 'F':
                    return {Code::End, 0};
                default:
                    return {Code::Unknown, 0};
                }
            }
            if (final == '~')
            {
                if (params == "1" || params == "7")
                    return {Code::Home, 0};
                if (params == "4" || params == "8")
                    return {Code::End, 0};
                if (params == "3")
                    return {Code::Delete, 0};
                return {Code::Unknown, 0};
            }
            if (final == 'C' || final == 'D')
            {
                // Ctrl ("1;5" or "5") and Alt ("1;3" or "3") word movement.
                if (params == "1;5" || params == "5" || params == "1;3" || params == "3")
                    return {final == 'C' ? Code::WordRight : Code::WordLeft, 0};
            }
            return {Code::Unknown, 0};
        }

        /// @brief Read one byte, waiting at most a short interval.
        /// @param out  Receives the byte when available.
        /// @return true when a byte was read.
        static bool read_with_timeout(char &out)
        {
            pollfd pfd{};
            pfd.fd = STDIN_FILENO;
            pfd.events = POLLIN;
            // A signal (EINTR) is retried, not mistaken for the 30 ms timeout.
            const int rc =
                pjh::cli::detail::retry_on_eintr([&pfd] { return ::poll(&pfd, 1, 30); });
            if (rc <= 0)
                return false;
            const auto n = pjh::cli::detail::retry_on_eintr(
                [&out] { return ::read(STDIN_FILENO, &out, 1); });
            return n == 1;
        }

        std::ostream &m_output;
        std::optional<pjh::platform::ConsoleMode> m_mode;
    };

#endif
}  // namespace

namespace pjh::cli
{
    std::unique_ptr<ITerminal> make_tty_terminal(
        std::istream &input, std::ostream &output)
    {
        const bool stdin_tty = pjh::platform::Console::is_tty(0);
        const bool stdout_tty = pjh::platform::Console::is_tty(1);
        if (!detail::raw_mode_available(
                &input == &std::cin, &output == &std::cout, stdin_tty, stdout_tty))
            return nullptr;
#if defined(_WIN32)
        return std::make_unique<WindowsTerminal>(output);
#else
        return std::make_unique<PosixTerminal>(output);
#endif
    }
}  // namespace pjh::cli
