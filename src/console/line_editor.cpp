#include <algorithm>
#include <cctype>
#include <cstddef>
#include <pjh_cli/console/history.hpp>
#include <pjh_cli/console/line_editor.hpp>
#include <pjh_cli/detail/string_utils.hpp>
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

        /// @brief True for a word character: ASCII alphanumeric, '_', or any
        ///        byte >= 0x80 (so every byte of a multi-byte code point counts,
        ///        keeping CJK sequences together while stepping by code point).
        bool is_word_char(unsigned char c) noexcept
        {
            return std::isalnum(c) != 0 || c == '_' || c >= 0x80;
        }

        /// @brief Byte offset of the start of the word at or before @p pos.
        ///
        /// Skips separator code points backwards, then word code points, so the
        /// result lands on a UTF-8 code-point boundary (never 0 on overflow).
        ///
        /// @param b    Byte string buffer.
        /// @param pos  Starting byte offset (code-point boundary).
        /// @return Byte offset of the previous word start.
        std::size_t word_left(std::string_view b, std::size_t pos) noexcept
        {
            std::size_t i = pos;
            while (i > 0)
            {
                const std::size_t prev = detail::utf8_prev_code_point(b, i);
                if (is_word_char(static_cast<unsigned char>(b[prev])))
                    break;
                i = prev;
            }
            while (i > 0)
            {
                const std::size_t prev = detail::utf8_prev_code_point(b, i);
                if (!is_word_char(static_cast<unsigned char>(b[prev])))
                    break;
                i = prev;
            }
            return i;
        }

        /// @brief Byte offset of the end of the word at or after @p pos.
        ///
        /// Skips separator code points forward, then word code points, so the
        /// result lands on a UTF-8 code-point boundary (never past the end).
        ///
        /// @param b    Byte string buffer.
        /// @param pos  Starting byte offset (code-point boundary).
        /// @return Byte offset of the next word end.
        std::size_t word_right(std::string_view b, std::size_t pos) noexcept
        {
            std::size_t i = pos;
            while (i < b.size() && !is_word_char(static_cast<unsigned char>(b[i])))
                i = detail::utf8_next_code_point(b, i);
            while (i < b.size() && is_word_char(static_cast<unsigned char>(b[i])))
                i = detail::utf8_next_code_point(b, i);
            return i;
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
        m_cursor = 0;
        m_cursor_cp = 0;
        m_rendered_len = 0;
        m_terminal.write(m_prompt);

        for (;;)
        {
            KeyEvent key = m_terminal.read_key();
            switch (key.code)
            {
            case KeyEvent::Code::Character:
                if (m_cursor == buffer.size())
                {
                    buffer.push_back(key.ch);
                    m_terminal.write(std::string_view(&key.ch, 1));
                    m_cursor = buffer.size();
                    if (!detail::is_utf8_continuation_byte(
                            static_cast<unsigned char>(key.ch)))
                    {
                        ++m_cursor_cp;
                        ++m_rendered_len;
                    }
                }
                else
                {
                    buffer.insert(m_cursor, 1, key.ch);
                    render(buffer, m_cursor + 1);
                }
                break;
            case KeyEvent::Code::Backspace:
                if (!buffer.empty())
                {
                    if (m_cursor == buffer.size())
                    {
                        buffer.resize(
                            detail::utf8_prev_code_point(buffer, buffer.size()));
                        m_terminal.erase_last(1);
                        m_cursor = buffer.size();
                        m_cursor_cp = detail::utf8_code_point_count(buffer);
                        m_rendered_len = m_cursor_cp;
                    }
                    else if (m_cursor > 0)
                    {
                        const std::size_t start =
                            detail::utf8_prev_code_point(buffer, m_cursor);
                        buffer.erase(start, m_cursor - start);
                        render(buffer, start);
                    }
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
            case KeyEvent::Code::Cancel:
                m_terminal.write("^C\n");
                buffer.clear();
                if (m_history)
                    m_history->reset_cursor();
                m_navigating = false;
                m_draft.clear();
                m_cursor = 0;
                m_cursor_cp = 0;
                m_rendered_len = 0;
                m_terminal.write(m_prompt);
                break;
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
            case KeyEvent::Code::Left:
                if (m_cursor > 0)
                    render(buffer, detail::utf8_prev_code_point(buffer, m_cursor));
                break;
            case KeyEvent::Code::Right:
            {
                const std::size_t next = detail::utf8_next_code_point(buffer, m_cursor);
                if (next != m_cursor)
                    render(buffer, next);
                break;
            }
            case KeyEvent::Code::Home:
                if (m_cursor != 0)
                    render(buffer, 0);
                break;
            case KeyEvent::Code::End:
                if (m_cursor != buffer.size())
                    render(buffer, buffer.size());
                break;
            case KeyEvent::Code::Delete:
                if (m_cursor < buffer.size())
                {
                    const std::size_t next =
                        detail::utf8_next_code_point(buffer, m_cursor);
                    buffer.erase(m_cursor, next - m_cursor);
                    render(buffer, m_cursor);
                }
                break;
            case KeyEvent::Code::WordLeft:
            {
                const std::size_t target = word_left(buffer, m_cursor);
                if (target != m_cursor)
                    render(buffer, target);
                break;
            }
            case KeyEvent::Code::WordRight:
            {
                const std::size_t target = word_right(buffer, m_cursor);
                if (target != m_cursor)
                    render(buffer, target);
                break;
            }
            case KeyEvent::Code::Unknown:
                break;
            }
        }
    }

    void LineEditor::render(std::string_view new_buffer, std::size_t new_cursor)
    {
        const std::size_t new_cursor_cp =
            detail::utf8_code_point_count(new_buffer.substr(0, new_cursor));
        const std::size_t new_len = detail::utf8_code_point_count(new_buffer);

        m_terminal.move_cursor_left(m_cursor_cp);
        m_terminal.write(new_buffer);
        if (m_rendered_len > new_len)
            m_terminal.write(std::string(m_rendered_len - new_len, ' '));
        const std::size_t placed = std::max(m_rendered_len, new_len);
        m_terminal.move_cursor_left(placed - new_cursor_cp);

        m_cursor = new_cursor;
        m_cursor_cp = new_cursor_cp;
        m_rendered_len = new_len;
    }

    void LineEditor::handle_tab(
        std::string &buffer, const CompletionFn &complete, const HintFn &hint)
    {
        const CompletionResult result = complete(buffer, m_cursor);
        const auto &candidates = result.candidates;
        if (candidates.size() == 1)
        {
            const std::string &candidate = candidates.front().display;
            if (candidate.size() > result.prefix_len)
            {
                std::string remainder = candidate.substr(result.prefix_len);
                if (m_cursor == buffer.size())
                {
                    buffer += remainder;
                    m_terminal.write(remainder);
                    m_cursor = buffer.size();
                    m_cursor_cp = detail::utf8_code_point_count(buffer);
                    m_rendered_len = m_cursor_cp;
                }
                else
                {
                    buffer.insert(m_cursor, remainder);
                    render(buffer, m_cursor + remainder.size());
                }
            }
            if (m_cursor == buffer.size() && (buffer.empty() || buffer.back() != ' '))
            {
                buffer.push_back(' ');
                m_terminal.write(" ");
                m_cursor = buffer.size();
                ++m_cursor_cp;
                ++m_rendered_len;
            }
            return;
        }

        show_candidates(candidates, buffer);
        show_hint(hint, buffer);
    }

    void LineEditor::show_hint(const HintFn &hint, std::string_view buffer)
    {
        std::string text = hint(buffer, m_cursor);
        if (!text.empty())
            m_terminal.write("\n" + text + "\n");
        m_terminal.write(m_prompt + std::string(buffer));
        const std::size_t len = detail::utf8_code_point_count(buffer);
        m_terminal.move_cursor_left(len - m_cursor_cp);
        m_rendered_len = len;
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
        auto entry = m_history->prev();
        if (entry.is_some())
        {
            if (!m_navigating)
            {
                m_draft = buffer;
                m_navigating = true;
            }
            replace_buffer(buffer, entry.unwrap());
        }
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
        // History recall always leaves the cursor at the end.  If the visible
        // cursor is mid-line, first redraw it to the end so the erase below
        // does not overrun the prompt.
        if (m_cursor != buffer.size())
            render(buffer, buffer.size());

        if (buffer != text)
        {
            m_terminal.erase_last(detail::utf8_code_point_count(buffer));
            buffer.assign(text);
            m_terminal.write(buffer);
        }
        m_cursor = buffer.size();
        m_cursor_cp = detail::utf8_code_point_count(buffer);
        m_rendered_len = m_cursor_cp;
    }

}  // namespace pjh::cli
