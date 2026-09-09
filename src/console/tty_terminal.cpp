#include <cstddef>
#include <iostream>
#include <memory>
#include <ostream>
#include <pjh_cli/console/line_editor.hpp>
#include <pjh_cli/detail/io_retry.hpp>
#include <string_view>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <conio.h>
#include <io.h>
#include <windows.h>

#include <cstdio>
#else
#include <poll.h>
#include <termios.h>
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
            HANDLE input = ::GetStdHandle(STD_INPUT_HANDLE);
            if (input == INVALID_HANDLE_VALUE || input == nullptr)
                return;
            if (!::GetConsoleMode(input, &m_saved))
                return;
            m_saved_valid = true;
            apply_raw();
        }

        ~WindowsTerminal() override { suspend(); }

        void suspend() override
        {
            if (!m_active)
                return;
            HANDLE input = ::GetStdHandle(STD_INPUT_HANDLE);
            if (input == INVALID_HANDLE_VALUE || input == nullptr)
                return;
            if (::SetConsoleMode(input, m_saved))
                m_active = false;
        }

        void resume() override
        {
            if (m_active || !m_saved_valid)
                return;
            apply_raw();
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
        /// @brief Disable echo, line input, and processed input so _getch()
        ///        sees every key, including Ctrl-C.
        void apply_raw()
        {
            HANDLE input = ::GetStdHandle(STD_INPUT_HANDLE);
            if (input == INVALID_HANDLE_VALUE || input == nullptr)
                return;
            DWORD raw = m_saved &
                        ~(ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT | ENABLE_PROCESSED_INPUT);
            if (::SetConsoleMode(input, raw))
                m_active = true;
        }

        std::ostream &m_output;
        DWORD m_saved = 0;
        bool m_saved_valid = false;
        bool m_active = false;
    };

#else

    /// @brief POSIX raw-mode backend built on termios + read().
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
            if (::tcgetattr(STDIN_FILENO, &m_saved) != 0)
                return;
            m_saved_valid = true;
            apply_raw();
        }

        ~PosixTerminal() override { suspend(); }

        void suspend() override
        {
            if (!m_active)
                return;
            if (::tcsetattr(STDIN_FILENO, TCSANOW, &m_saved) == 0)
                m_active = false;
        }

        void resume() override
        {
            if (m_active || !m_saved_valid)
                return;
            apply_raw();
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

        /// @brief Disable canonical mode, echo, and signal generation so read()
        ///        sees every key, including Ctrl-C.
        void apply_raw()
        {
            termios raw = m_saved;
            raw.c_lflag &= static_cast<tcflag_t>(~(ICANON | ECHO | ISIG));
            raw.c_cc[VMIN] = 1;
            raw.c_cc[VTIME] = 0;
            if (::tcsetattr(STDIN_FILENO, TCSANOW, &raw) == 0)
                m_active = true;
        }

        std::ostream &m_output;
        termios m_saved{};
        bool m_saved_valid = false;
        bool m_active = false;
    };

#endif
}  // namespace

namespace pjh::cli
{
    std::unique_ptr<ITerminal> make_tty_terminal(
        std::istream &input, std::ostream &output)
    {
#if defined(_WIN32)
        const bool stdin_tty = ::_isatty(::_fileno(stdin)) != 0;
        const bool stdout_tty = ::_isatty(::_fileno(stdout)) != 0;
#else
        const bool stdin_tty = ::isatty(STDIN_FILENO) != 0;
        const bool stdout_tty = ::isatty(STDOUT_FILENO) != 0;
#endif
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
