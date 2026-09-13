#include <cstddef>
#include <iostream>
#include <memory>
#include <optional>
#include <ostream>
#include <pjh_cli/console/line_editor.hpp>
#include <pjh_cli/detail/io_retry.hpp>
#include <pjh_platform/console.hpp>
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
                int ext = ::_getch();
                if (ext == 'H')
                    return {pjh::cli::KeyEvent::Code::Up, 0};
                if (ext == 'P')
                    return {pjh::cli::KeyEvent::Code::Down, 0};
                return {pjh::cli::KeyEvent::Code::Unknown, 0};
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
        /// @brief Decode an ESC-prefixed arrow sequence without blocking on a
        ///        bare ESC (short poll between bytes).
        pjh::cli::KeyEvent read_escape()
        {
            char first = 0;
            if (!read_with_timeout(first))
                return {pjh::cli::KeyEvent::Code::Unknown, 0};
            if (first != '[' && first != 'O')
                return {pjh::cli::KeyEvent::Code::Unknown, 0};

            char second = 0;
            if (!read_with_timeout(second))
                return {pjh::cli::KeyEvent::Code::Unknown, 0};
            if (second == 'A')
                return {pjh::cli::KeyEvent::Code::Up, 0};
            if (second == 'B')
                return {pjh::cli::KeyEvent::Code::Down, 0};
            return {pjh::cli::KeyEvent::Code::Unknown, 0};
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
