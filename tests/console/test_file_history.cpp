#include <doctest/doctest.h>

#include <atomic>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <pjh_cli/app.hpp>
#include <pjh_cli/console.hpp>
#include <pjh_cli/console/file_history.hpp>
#include <pjh_cli/console/ring_buffer_history.hpp>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>

using namespace pjh::cli;

namespace
{
    namespace fs = std::filesystem;

    /// @brief Unique temp path removed on destruction (no leakage).
    class TempHistoryFile
    {
    public:
        TempHistoryFile()
        {
            static std::atomic<unsigned> counter{0};
            const auto stamp =
                std::chrono::steady_clock::now().time_since_epoch().count();
            m_path =
                fs::temp_directory_path() / ("pjh_cli_hist_" + std::to_string(stamp) +
                                             "_" + std::to_string(counter++) + ".txt");
        }
        ~TempHistoryFile()
        {
            std::error_code ec;
            fs::remove(m_path, ec);
        }
        const fs::path &path() const noexcept { return m_path; }

    private:
        fs::path m_path;
    };

    void write_file(const fs::path &p, std::string_view content)
    {
        std::ofstream out(p, std::ios::trunc | std::ios::binary);
        out << content;
    }

    std::string read_file(const fs::path &p)
    {
        std::ifstream in(p, std::ios::binary);
        return std::string(std::istreambuf_iterator<char>(in), {});
    }
}  // namespace

// ── Load / format ──

TEST_CASE("FileHistory loads existing entries")
{
    TempHistoryFile file;
    write_file(file.path(), "one\ntwo\nthree\n");

    FileHistory h(file.path());
    CHECK(h.size() == 3);

    auto v = h.prev();
    CHECK(v.is_some());
    CHECK(v.unwrap() == "three");
    v = h.prev();
    CHECK(v.unwrap() == "two");
    v = h.prev();
    CHECK(v.unwrap() == "one");
    CHECK(h.prev().is_none());
}

TEST_CASE("FileHistory missing file starts empty")
{
    TempHistoryFile file;
    FileHistory h(file.path());
    CHECK(h.size() == 0);
    CHECK(h.prev().is_none());
    CHECK(h.next().is_none());
    CHECK_FALSE(fs::exists(file.path()));  // created on first write only

    h.push("first");
    CHECK(fs::exists(file.path()));
    CHECK(read_file(file.path()) == "first\n");
}

TEST_CASE("FileHistory skips blank lines on load")
{
    TempHistoryFile file;
    write_file(file.path(), "a\n\nb\n");

    FileHistory h(file.path());
    CHECK(h.size() == 2);
    CHECK(h.prev().unwrap() == "b");
    CHECK(h.prev().unwrap() == "a");
    CHECK(read_file(file.path()) == "a\nb\n");
}

TEST_CASE("FileHistory strips trailing carriage return on load")
{
    TempHistoryFile file;
    write_file(file.path(), "a\r\nb\r\n");

    FileHistory h(file.path());
    CHECK(h.size() == 2);
    CHECK(h.prev().unwrap() == "b");
    CHECK(h.prev().unwrap() == "a");
    // The on-disk CRLF endings are left untouched; stripping is load-time only.
    CHECK(read_file(file.path()) == "a\r\nb\r\n");
}

TEST_CASE("FileHistory collapses consecutive duplicates on load and normalizes the file")
{
    TempHistoryFile file;
    write_file(file.path(), "a\na\nb\n");

    FileHistory h(file.path());
    CHECK(h.size() == 2);
    CHECK(h.prev().unwrap() == "b");
    CHECK(h.prev().unwrap() == "a");
    CHECK(read_file(file.path()) == "a\nb\n");
}

TEST_CASE("FileHistory enforces max_entries on load and rewrites the file")
{
    TempHistoryFile file;
    write_file(file.path(), "a\nb\nc\nd\ne\n");

    FileHistory h(file.path(), 3);
    CHECK(h.size() == 3);
    CHECK(h.prev().unwrap() == "e");
    CHECK(h.prev().unwrap() == "d");
    CHECK(h.prev().unwrap() == "c");
    CHECK(h.prev().is_none());
    CHECK(read_file(file.path()) == "c\nd\ne\n");
}

// ── Persistence / cap ──

TEST_CASE("FileHistory append persists each push immediately")
{
    TempHistoryFile file;
    FileHistory h(file.path());
    h.push("a");
    CHECK(read_file(file.path()) == "a\n");
    h.push("b");
    CHECK(read_file(file.path()) == "a\nb\n");

    FileHistory again(file.path());
    CHECK(again.size() == 2);
    CHECK(again.prev().unwrap() == "b");
}

TEST_CASE("FileHistory trims oldest entry when exceeding the cap")
{
    TempHistoryFile file;
    FileHistory h(file.path(), 2);
    h.push("a");
    h.push("b");
    CHECK(read_file(file.path()) == "a\nb\n");

    h.push("c");
    CHECK(h.size() == 2);
    CHECK(read_file(file.path()) == "b\nc\n");

    CHECK(h.prev().unwrap() == "c");
    CHECK(h.prev().unwrap() == "b");
    CHECK(h.prev().is_none());
}

TEST_CASE("FileHistory with max_entries zero is unlimited")
{
    TempHistoryFile file;
    FileHistory h(file.path(), 0);
    CHECK(h.max_entries() == 0);
    for (int i = 0; i < 5; ++i) h.push("line" + std::to_string(i));
    CHECK(h.size() == 5);
    CHECK(h.prev().unwrap() == "line4");

    // Deliberate divergence: RingBufferHistory(0) throws instead.
    CHECK_THROWS_AS(RingBufferHistory(0), std::invalid_argument);
}

TEST_CASE("FileHistory clear empties memory and truncates the file")
{
    TempHistoryFile file;
    FileHistory h(file.path());
    h.push("a");
    h.push("b");
    CHECK(read_file(file.path()) == "a\nb\n");

    h.clear();
    CHECK(h.size() == 0);
    CHECK(h.prev().is_none());
    CHECK(read_file(file.path()).empty());
}

TEST_CASE("FileHistory save rewrites the file from memory")
{
    TempHistoryFile file;
    FileHistory h(file.path());
    h.push("a");
    write_file(file.path(), "x\ny\n");  // corrupt externally

    CHECK(h.save());
    CHECK(read_file(file.path()) == "a\n");
}

TEST_CASE("FileHistory empty push is ignored")
{
    TempHistoryFile file;
    FileHistory h(file.path());
    h.push("");
    CHECK(h.size() == 0);
    CHECK_FALSE(fs::exists(file.path()));
}

TEST_CASE("FileHistory consecutive duplicate dedup")
{
    TempHistoryFile file;
    FileHistory h(file.path());
    h.push("a");
    h.push("a");
    h.push("b");

    CHECK(h.size() == 2);
    CHECK(read_file(file.path()) == "a\nb\n");
    CHECK(h.prev().unwrap() == "b");
    CHECK(h.prev().unwrap() == "a");
}

// ── Navigation (mirrors the RingBufferHistory cases) ──

TEST_CASE("FileHistory prev and next navigation")
{
    TempHistoryFile file;
    FileHistory h(file.path());
    h.push("x");
    h.push("y");
    h.push("z");

    (void)h.prev();  // z
    (void)h.prev();  // y
    (void)h.prev();  // x

    auto v = h.next();
    CHECK(v.is_some());
    CHECK(v.unwrap() == "y");
    v = h.next();
    CHECK(v.unwrap() == "z");
    CHECK(h.next().is_none());

    h.reset_cursor();
    v = h.prev();
    CHECK(v.is_some());
    CHECK(v.unwrap() == "z");
}

TEST_CASE("FileHistory cursor reset after push beyond capacity")
{
    TempHistoryFile file;
    FileHistory h(file.path(), 2);
    h.push("a");
    h.push("b");
    h.push("c");
    CHECK(h.size() == 2);

    auto v = h.prev();
    CHECK(v.unwrap() == "c");
    CHECK(h.next().is_none());
}

// ── Error handling ──

TEST_CASE("FileHistory unwritable path does not throw")
{
    TempHistoryFile file;
    const fs::path missing =
        file.path().parent_path() / "pjh_cli_no_such_dir" / "history.txt";
    FileHistory h(missing);
    CHECK(h.size() == 0);
    h.push("cmd");
    CHECK(h.size() == 1);   // memory still records the entry
    CHECK_FALSE(h.save());  // I/O failure reported, no exception
    CHECK(h.prev().unwrap() == "cmd");
}

// ── Integration with InteractiveConsole ──

TEST_CASE("InteractiveConsole persists history through FileHistory")
{
    TempHistoryFile file;
    {
        App app("test", "1.0", "File history");
        int called = 0;
        app.action(
            [&called](ParseContext &) -> CliResult<void>
            {
                ++called;
                return CliResult<void>::Ok();
            });
        std::stringstream input, output, error;
        InteractiveConsole console(
            app, "> ", input, output, error, {}, {},
            std::make_unique<FileHistory>(file.path()));
        CHECK(console.process_line("do-something").is_ok());
        CHECK(called == 1);
    }
    FileHistory reloaded(file.path());  // a second session
    CHECK(reloaded.size() == 1);
    CHECK(reloaded.prev().unwrap() == "do-something");
}

TEST_CASE("InteractiveConsole persists parse-failed lines through FileHistory")
{
    TempHistoryFile file;
    {
        App app("test", "1.0", "File history err");
        std::stringstream input, output, error;
        InteractiveConsole console(
            app, "> ", input, output, error, {}, {},
            std::make_unique<FileHistory>(file.path()));
        CHECK(console.process_line("--bogus").is_err());
    }
    FileHistory reloaded(file.path());
    CHECK(reloaded.size() == 1);
    CHECK(reloaded.prev().unwrap() == "--bogus");
}

TEST_CASE("InteractiveConsole run loop persists each line through FileHistory")
{
    TempHistoryFile file;
    {
        App app("test", "1.0", "File history run");
        std::stringstream input("alpha\nbeta\nquit\n"), output, error;
        InteractiveConsole console(
            app, "> ", input, output, error, {}, {},
            std::make_unique<FileHistory>(file.path()));
        console.run();
    }
    FileHistory reloaded(file.path());
    CHECK(reloaded.size() == 2);
    CHECK(reloaded.prev().unwrap() == "beta");
    CHECK(reloaded.prev().unwrap() == "alpha");
}
