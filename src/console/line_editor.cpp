#include <cstddef>
#include <pjh_cli/console/history.hpp>
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

    LineEditor::LineEditor(ITerminal &terminal, std::string prompt, IHistory *history) :
        m_terminal(terminal), m_prompt(std::move(prompt)), m_history(history)
    {
    }

    bool LineEditor::read_line(
        std::string &out, const CompletionFn &complete, const HintFn &hint)
    {
        std::string buffer;
        if (m_history)
            m_history->reset_cursor();
        m_navigating = false;
        m_draft.clear();
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
                m_navigating = false;
                m_draft.clear();
                return true;
            case KeyEvent::Code::Eof:
                m_terminal.write("\n");
                m_navigating = false;
                m_draft.clear();
                return false;
            case KeyEvent::Code::Up:
                recall_prev(buffer);
                break;
            case KeyEvent::Code::Down:
                recall_next(buffer);
                break;
            case KeyEvent::Code::Unknown:
                break;
            }
        }
    }

    void LineEditor::handle_tab(
        std::string &buffer, const CompletionFn &complete, const HintFn &hint)
    {
        const CompletionResult result = complete(buffer, buffer.size());
        const auto &candidates = result.candidates;
        if (candidates.size() == 1)
        {
            const std::string &candidate = candidates.front().display;
            if (candidate.size() > result.prefix_len)
            {
                std::string remainder = candidate.substr(result.prefix_len);
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

    void LineEditor::recall_prev(std::string &buffer)
    {
        if (!m_history)
            return;
        if (!m_navigating)
        {
            m_draft = buffer;
            m_navigating = true;
        }
        auto entry = m_history->prev();
        if (entry.is_some())
            replace_buffer(buffer, entry.unwrap());
    }

    void LineEditor::recall_next(std::string &buffer)
    {
        if (!m_history)
            return;
        auto entry = m_history->next();
        if (entry.is_some())
        {
            replace_buffer(buffer, entry.unwrap());
            return;
        }
        if (m_navigating)
        {
            replace_buffer(buffer, m_draft);
            m_navigating = false;
        }
    }

    void LineEditor::replace_buffer(std::string &buffer, std::string_view text)
    {
        if (buffer == text)
            return;
        m_terminal.erase_last(buffer.size());
        buffer.assign(text);
        m_terminal.write(buffer);
    }

}  // namespace pjh::cli
