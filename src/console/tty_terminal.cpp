#include <cstddef>
#include <iostream>
#include <memory>
#include <ostream>
#include <pjh_cli/console/line_editor.hpp>
#include <string_view>

#if defined(_WIN32)
#include <conio.h>
#include <io.h>

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
    class WindowsTerminal final : public pjh::cli::ITerminal
    {
    public:
        /// @param output  Stream used for echo.
        explicit WindowsTerminal(std::ostream &output) : m_output(output) {}

        pjh::cli::KeyEvent read_key() override
        {
            int c = ::_getch();
            if (c == 0x03 || c == 0x04)  // Ctrl-C / Ctrl-D.
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
            for (std::size_t i = 0; i < count; ++i) m_output << "\b \b";
            m_output << std::flush;
        }

    private:
        std::ostream &m_output;
    };

#else

    /// @brief POSIX raw-mode backend built on termios + read().
    ///
    /// Saves the terminal attributes on construction and restores them in the
    /// destructor, so every exit path (EOF, quit, exception) leaves the shell
    /// in a usable state.  Restoration only happens when the terminal was
    /// actually switched to raw mode.
    class PosixTerminal final : public pjh::cli::ITerminal
    {
    public:
        /// @param output  Stream used for echo.
        explicit PosixTerminal(std::ostream &output) : m_output(output)
        {
            if (::tcgetattr(STDIN_FILENO, &m_saved) != 0)
                return;
            termios raw = m_saved;
            raw.c_lflag &= static_cast<tcflag_t>(~(ICANON | ECHO));
            raw.c_cc[VMIN] = 1;
            raw.c_cc[VTIME] = 0;
            if (::tcsetattr(STDIN_FILENO, TCSANOW, &raw) == 0)
                m_active = true;
        }

        ~PosixTerminal() override
        {
            if (m_active)
                ::tcsetattr(STDIN_FILENO, TCSANOW, &m_saved);
        }

        PosixTerminal(const PosixTerminal &) = delete;
        PosixTerminal &operator=(const PosixTerminal &) = delete;

        pjh::cli::KeyEvent read_key() override
        {
            char c = 0;
            if (::read(STDIN_FILENO, &c, 1) != 1)
                return {pjh::cli::KeyEvent::Code::Eof, 0};

            switch (c)
            {
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
            if (::poll(&pfd, 1, 30) <= 0)
                return false;
            return ::read(STDIN_FILENO, &out, 1) == 1;
        }

        std::ostream &m_output;
        termios m_saved{};
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
        if (&input != &std::cin || !::_isatty(::_fileno(stdin)))
            return nullptr;
        return std::make_unique<WindowsTerminal>(output);
#else
        if (&input != &std::cin || !::isatty(STDIN_FILENO))
            return nullptr;
        return std::make_unique<PosixTerminal>(output);
#endif
    }
}  // namespace pjh::cli
