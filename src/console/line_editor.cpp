#include <cstddef>
#include <pjh_cli/console/line_editor.hpp>
#include <pjh_cli/format/info.hpp>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace pjh::cli
{
    namespace
    {
        /// @brief Length of the trailing non-space run of @p buffer.
        std::size_t trailing_token_length(std::string_view buffer)
        {
            std::size_t len = 0;
            for (std::size_t i = buffer.size(); i > 0 && buffer[i - 1] != ' '; --i) ++len;
            return len;
        }

        /// @brief Join candidate displays with two spaces.
        std::string join_candidates(const std::vector<CompletionCandidate> &candidates)
        {
            std::string out;
            for (std::size_t i = 0; i < candidates.size(); ++i)
            {
                if (i != 0)
                    out += "  ";
                out += candidates[i].display;
            }
            return out;
        }
    }  // namespace

    LineEditor::LineEditor(ITerminal &terminal, std::string prompt) :
        m_terminal(terminal), m_prompt(std::move(prompt))
    {
    }

    bool LineEditor::read_line(
        std::string &out, const CompletionFn &complete, const HintFn &hint)
    {
        std::string buffer;
        m_terminal.write(m_prompt);

        for (;;)
        {
            KeyEvent key = m_terminal.read_key();
            switch (key.code)
            {
            case KeyEvent::Code::Character:
                buffer.push_back(key.ch);
                m_terminal.write(std::string_view(&key.ch, 1));
                break;
            case KeyEvent::Code::Backspace:
                if (!buffer.empty())
                {
                    buffer.pop_back();
                    m_terminal.erase_last(1);
                }
                break;
            case KeyEvent::Code::Tab:
                handle_tab(buffer, complete, hint);
                break;
            case KeyEvent::Code::Enter:
                m_terminal.write("\n");
                out = std::move(buffer);
                return true;
            case KeyEvent::Code::Eof:
                m_terminal.write("\n");
                return false;
            case KeyEvent::Code::Up:
            case KeyEvent::Code::Down:
            case KeyEvent::Code::Unknown:
                // task 14 seam: history navigation owns Up/Down; for now no-op.
                break;
            }
        }
    }

    void LineEditor::handle_tab(
        std::string &buffer, const CompletionFn &complete, const HintFn &hint)
    {
        const auto candidates = complete(buffer, buffer.size());
        if (candidates.size() == 1)
        {
            const std::size_t prefix_len = trailing_token_length(buffer);
            const std::string &candidate = candidates.front().display;
            if (candidate.size() > prefix_len)
            {
                std::string remainder = candidate.substr(prefix_len);
                buffer += remainder;
                m_terminal.write(remainder);
            }
            if (buffer.empty() || buffer.back() != ' ')
            {
                buffer.push_back(' ');
                m_terminal.write(" ");
            }
            return;
        }

        show_candidates(candidates, buffer);
        show_hint(hint, buffer);
    }

    void LineEditor::show_hint(const HintFn &hint, std::string_view buffer)
    {
        std::string text = hint(buffer, buffer.size());
        if (!text.empty())
            m_terminal.write("\n" + text + "\n");
        m_terminal.write(m_prompt + std::string(buffer));
    }

    void LineEditor::show_candidates(
        const std::vector<CompletionCandidate> &candidates, std::string_view)
    {
        if (candidates.empty())
            return;
        m_terminal.write("\n" + join_candidates(candidates) + "\n");
    }

}  // namespace pjh::cli
